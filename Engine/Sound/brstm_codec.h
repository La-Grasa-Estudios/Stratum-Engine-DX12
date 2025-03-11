#pragma once

#include "znmsp.h"

#include <fstream>
#include <vector>
#include <mutex>

#include "adpcm-lib/wave.h"
#include "adpcm-lib/adpcm.h"
#include "VFS/ZVFS.h"

BEGIN_ENGINE

namespace BRSTM {

	struct BRSTMHeader {

		uint32_t header;
		uint8_t ByteMark;
		uint16_t Version;
		uint32_t Size;
		uint16_t nbChunks;
		uint8_t loop;
		uint32_t loopStart;
		uint32_t loopSize;

		

		bool is_valid() {

			if (header != 0x5253544D) {
				return false;
			}

			return true;

		}

		void construct() {

			header = 0x5253544D;
			ByteMark = 0xFE;
			Version = 0x0002;

		}

		uint32_t get_header_size() {
			return (4 + 1 + 2 + 4 + 2 + 1 + 8 + 2);
		}

	};

	struct WAVEData {
		WAVEHeader header;
		char* data;
		void free() {
			delete[] data;
		}
	};

	struct BRSTMChunk {

		uint32_t header = 0x68437453;
		ADPCMHeader adpcmHeader;
		WAVEHeader waveHeader;
		char* data = NULL;
		uint32_t chunk_size = NULL;
		uint32_t next_chunk = NULL;
		
		uint32_t get_chunk_size() {
			return 4 + sizeof(ADPCMHeader) + sizeof(WAVEHeader) + 8 + chunk_size;
		}

		uint32_t get_chunk_samples() {
			return (waveHeader.subchunk2Size / (waveHeader.bitsPerSample / 8) / waveHeader.numChannels);
		}

		bool is_header_valid() {
			return header == 0x68437453;
		}

		void read(ENGINE_NAMESPACE::PakStream in, const BRSTMHeader& header);

		void write(std::ofstream& out);

		WAVEData get_samples();

		void from_samples(WAVEHeader& header, std::ifstream& in);

		~BRSTMChunk() {
			free();
		}

		void free() {
			if (!data) return;
			delete[] data;
			data = 0;
		}

	};

	struct BRSTMFile {

		PakStream file_stream = NULL;
		BRSTMHeader header;

		std::mutex mutex;

		BRSTMFile(const char* file);

		BRSTMChunk* get_chunk(uint32_t& position, uint32_t& positionInSamples, bool& has_chunks_avaible);

		void rewind(uint32_t& position, bool& has_chunks_available);

	};

	struct BRSTMStream {

		BRSTMStream(BRSTMFile* pFile);

		BRSTMChunk* get_next_chunk();

		void rewind();

		bool has_chunks_available;
		uint32_t filePosition = 0;
		uint32_t positionInSamples = 0;
		BRSTMFile* file;

	};

	

	static void to_brstm(const char* waveFile, uint32_t loopStart, uint32_t loopEnd) {

		std::ifstream in(waveFile, std::ios::binary);

		WAVEHeader header;

		std::vector<BRSTMChunk*> brstmChunks;
		BRSTMHeader brstmHeader;
		brstmHeader.construct();

		in.read((char*)&header, sizeof(WAVEHeader));

		uint16_t nbChunks = 0;

		while (!in.eof()) {
			nbChunks++;
			BRSTMChunk* chunk = new BRSTMChunk();
			chunk->from_samples(header, in);
			brstmChunks.push_back(chunk);
		}

		std::string out = std::string(waveFile).append(".brstm");
		std::ofstream outFile(out, std::ios::binary);

		uint32_t offset = brstmHeader.get_header_size();

		uint32_t samples = 0;

		for (int i = 0; i < brstmChunks.size(); i++) {

			BRSTMChunk* chunk = brstmChunks[i];
			BRSTMChunk* next = NULL;

			if (loopEnd != 0 && samples > loopEnd) {
				std::cout << "Skipping chunk after loop end \n";
				nbChunks--;
				continue;
			}

			if (i + 1 < nbChunks) {

				next = brstmChunks[i + 1];

			}

			if (next) {

				offset = chunk->get_chunk_size() + offset;

				chunk->next_chunk = offset;

				std::cout << "Next chunk is at position " << offset << " current chunk is in " << (offset - chunk->get_chunk_size()) << "\n";

			}

			samples += chunk->get_chunk_samples();


		}

		brstmHeader.loop = loopEnd != 0;
		brstmHeader.loopStart = loopStart;
		brstmHeader.loopSize = loopEnd;
		brstmHeader.nbChunks = nbChunks;
		brstmHeader.Size = 0x69;

		outFile.write((char*)&brstmHeader, sizeof(BRSTMHeader));

		for (int i = 0; i < nbChunks; i++) {
			BRSTMChunk* chunk = brstmChunks[i];
			chunk->write(outFile);
		}

	}

	static void to_brstm(const char* waveFile) {
		to_brstm(waveFile, 0U, 0U);
	}

}

END_ENGINE
