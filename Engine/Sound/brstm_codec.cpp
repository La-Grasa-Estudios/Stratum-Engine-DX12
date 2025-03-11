#include "brstm_codec.h"

using namespace ENGINE_NAMESPACE::BRSTM;

void BRSTMChunk::read(PakStream in, const BRSTMHeader& header) {

	if (data) {
		delete[] data;
	}

	in->read((char*)&header, 4);
	in->read(&adpcmHeader, sizeof(ADPCMHeader));
	in->read(&waveHeader, sizeof(WAVEHeader));
	in->read(&chunk_size, sizeof(uint32_t));
	in->read(&next_chunk, sizeof(uint32_t));

	data = new char[chunk_size];

	in->read(data, chunk_size);

}

void BRSTMChunk::write(std::ofstream& out) {

	out.write((char*)&header, 4);
	out.write(reinterpret_cast <char*>(&adpcmHeader), sizeof(ADPCMHeader));
	out.write(reinterpret_cast <char*>(&waveHeader), sizeof(WAVEHeader));
	out.write(reinterpret_cast<char*>(&chunk_size), sizeof(uint32_t));
	out.write(reinterpret_cast<char*>(&next_chunk), sizeof(uint32_t));
	out.write(data, chunk_size);

}

WAVEData BRSTMChunk::get_samples() {

	char* samples = new char[waveHeader.subchunk2Size];

	memset(samples, 0, waveHeader.subchunk2Size);

	decompress(data, samples, adpcmHeader);

	WAVEData waveData;
	waveData.data = samples;
	waveData.header = waveHeader;

	return waveData;

}

void BRSTMChunk::from_samples(WAVEHeader& header, std::ifstream& in) {

	ADPCMHeader adpcm;

	memcpy(&waveHeader, &header, sizeof(WAVEHeader));

	uint32_t byteSize = waveHeader.byteRate;
	waveHeader.subchunk2Size = byteSize;

	std::vector<char> dataBlock(byteSize);

	uint32_t pos = 0;
	while (!in.eof() && in.read(dataBlock.data() + pos, 4) && pos < byteSize) {
		pos += 4;
	}

	if (waveHeader.subchunk2Size > (long)pos) {
		waveHeader.subchunk2Size = (long)pos;
	}

	uint32_t dataSize = ADPCMDataSize(waveHeader);
	char* adpcmData = new char[dataSize];
	compress(dataBlock.data(), adpcmData, waveHeader, adpcm);

	memcpy(&adpcmHeader, &adpcm, sizeof(ADPCMHeader));
	data = adpcmData;
	chunk_size = dataSize;

}

BRSTMFile::BRSTMFile(const char* file) {

	file_stream = ZVFS::GetFileStream(file);
	header = {};

	if (!file_stream->is_open()) {
		//Z_ERROR("Critical failure reading file stream: {}", file);
		return;
	}

	if (!file_stream->read(reinterpret_cast<char*>(&header), sizeof(BRSTMHeader)) && !header.is_valid()) {
		return;
	}

	if (header.nbChunks < 5) {
		//Z_INFO("Chunk count is {} loading fully loading {}", header.nbChunks, file);
	}

}

BRSTMChunk* BRSTMFile::get_chunk(uint32_t& position, uint32_t& positionInSamples, bool& has_chunks_avaible) {

	if (!file_stream) return NULL;

	std::scoped_lock lock(mutex);

	if (!has_chunks_avaible) return NULL;

	BRSTMChunk* next_chunk = new BRSTMChunk();

	file_stream->seekg(position);

	next_chunk->read(file_stream, header);

	if (!next_chunk->is_header_valid()) {
		next_chunk->free();
		return NULL;
	}

	has_chunks_avaible = next_chunk->next_chunk;

	if (!has_chunks_avaible) return NULL;

	uint32_t samples = next_chunk->get_chunk_samples();

	//std::cout << "Read " << samples << " samples" << std::endl;

	position += next_chunk->get_chunk_size();

	positionInSamples += samples; //(next_chunk->waveHeader.subchunk2Size / (next_chunk->waveHeader.bitsPerSample / 8) / next_chunk->waveHeader.numChannels);

	return next_chunk;

}

void BRSTMFile::rewind(uint32_t& position, bool& has_chunks_available) {

	position = header.get_header_size();
	has_chunks_available = true;

}

BRSTMStream::BRSTMStream(BRSTMFile* pFile) {
	file = pFile;
	if (!file) return;
	file->rewind(filePosition, has_chunks_available);
}

BRSTMChunk* BRSTMStream::get_next_chunk() {
	if (!file) return NULL;
	return file->get_chunk(filePosition, positionInSamples, has_chunks_available);
}

void BRSTMStream::rewind() {
	if (!file) return;
	positionInSamples = 0;
	file->rewind(filePosition, has_chunks_available);

}