#pragma once

#include "znmsp.h"

#include <istream>
#include <ostream>
#include <map>

#include "VFS/ZVFS.h"

BEGIN_ENGINE

struct SpfHeader
{
	char SpfIdentify[6]; // Must be SPFPKG 
	uint32_t Version;
	char XorMask[16];

	DLLEXPORT SpfHeader();

	DLLEXPORT bool IsValid();

	DLLEXPORT void Read(PakStream in);
	DLLEXPORT void Write(std::ostream& out) const;

	DLLEXPORT size_t Size();
};

struct SpfMetadataHeader
{
	uint64_t MetadataLen;
	uint64_t MetadataCount;

	DLLEXPORT  void Read(PakStream in);
	DLLEXPORT  void Write(std::ostream& out) const;

	DLLEXPORT size_t Size();
};

struct SpfMetadataValueKeyPair
{
	int32_t KeyLen;
	char* Key;
	int32_t ValueLen;
	char* Value;

	DLLEXPORT SpfMetadataValueKeyPair();
	DLLEXPORT SpfMetadataValueKeyPair(const std::string& key, const std::string& value);

	DLLEXPORT void Read(PakStream in);
	DLLEXPORT void Write(std::ostream& out) const;

	DLLEXPORT size_t Size();

	DLLEXPORT ~SpfMetadataValueKeyPair();
};

struct SpfFileIndex
{
	uint64_t FileMetaLen;
	uint64_t FileCount;

	DLLEXPORT void Read(PakStream in);
	DLLEXPORT void Write(std::ostream& out) const;

	DLLEXPORT size_t Size();
};

struct SpfFileMetadata
{
	uint8_t FilenameLen;
	char* Filename;
	uint64_t ContentsLen;
	uint64_t ContentIndex;

	DLLEXPORT SpfFileMetadata();
	DLLEXPORT SpfFileMetadata(const std::string& filename);

	DLLEXPORT void Read(PakStream in);
	DLLEXPORT void Write(std::ostream& out) const;

	DLLEXPORT size_t Size();

	DLLEXPORT ~SpfFileMetadata();
};

struct SpfFile
{
	SpfHeader Header;
	std::map<std::string, std::string> Metadata;
	std::map<std::string, Ref<SpfFileMetadata>> Files;

	PakStream Stream;

	DLLEXPORT SpfFile(const std::string& path);

	DLLEXPORT void ReadFile(SpfFileMetadata* pMetadata, char* dst);

	DLLEXPORT static void WriteFile(char* src, size_t size, std::ofstream& out, const SpfHeader& header);
	DLLEXPORT static void UnMaskFile(char* src, size_t size, const SpfHeader& header);

};

class SpfFileStream
{
public:

	DLLEXPORT SpfFileStream(const SpfFile& file, SpfFileMetadata* pFileMetadata);
	DLLEXPORT SpfFileStream(PakStream stream);

	DLLEXPORT size_t read(void* buffer, size_t size);
	DLLEXPORT void seekg(size_t pos, int mode = 0);
	DLLEXPORT void ignore(size_t size);
	DLLEXPORT size_t tellg();
	DLLEXPORT bool eof();
	DLLEXPORT bool is_open();
	DLLEXPORT bool is_self_contained();

private:
	PakStream m_PakStream;
	size_t m_StreamPosition;
	SpfFileMetadata* m_FileMetadata;
	SpfHeader m_FileHeader;
};

END_ENGINE