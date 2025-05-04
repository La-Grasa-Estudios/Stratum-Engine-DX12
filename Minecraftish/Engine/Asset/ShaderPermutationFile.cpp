#include "ShaderPermutationFile.h"

#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<int> s_UniformDistribution;

using namespace ENGINE_NAMESPACE;

void InitializeXorMask(char* xorMask)
{
	for (int i = 0; i < 16; i++)
	{
		char c = (char)(s_UniformDistribution(s_Engine));
		xorMask[i] = c;
	}
}

SpfFile::SpfFile(const std::string& path)
{

	PakStream stream = ZVFS::GetFileStream(path.c_str());

	this->Header.Read(stream);

	if (!Header.IsValid())
	{
		return;
	}

	SpfMetadataHeader metadata;
	metadata.Read(stream);

	for (int i = 0; i < metadata.MetadataCount; i++)
	{
		SpfMetadataValueKeyPair keyPair;
		keyPair.Read(stream);
		this->Metadata[keyPair.Key] = keyPair.Value;
	}

	SpfFileIndex fileIndex;
	fileIndex.Read(stream);

	for (int i = 0; i < fileIndex.FileCount; i++)
	{
		Ref<SpfFileMetadata> fileMeta = CreateRef<SpfFileMetadata>();
		fileMeta->Read(stream);
		if (this->Files.contains(fileMeta->Filename))
		{
			InfoLog("Collision!");
			continue;
		}
		this->Files[fileMeta->Filename] = fileMeta;
	}

	Stream = stream;

}

void SpfFile::ReadFile(SpfFileMetadata* pMetadata, char* dst)
{
	Stream->seekg(pMetadata->ContentIndex);
	Stream->read(dst, pMetadata->ContentsLen);
	UnMaskFile(dst, pMetadata->ContentsLen, Header);
}

void SpfFile::WriteFile(char* src, size_t size, std::ofstream& out, const SpfHeader& header)
{
	for (int i = 0; i < size; i++)
	{
		char xorKey = header.XorMask[i % 16] ^ (i & 0xFF);
		char c = src[i] ^ xorKey;
		out.write(&c, 1);
	}
}

void SpfFile::UnMaskFile(char* src, size_t size, const SpfHeader& header)
{
	for (int i = 0; i < size; i++)
	{
		char xorKey = header.XorMask[i % 16] ^ (i & 0xFF);
		src[i] ^= xorKey;
	}
}

SpfHeader::SpfHeader()
{
	memcpy(SpfIdentify, "SPFPKG", 6);
	Version = 1;
	InitializeXorMask(XorMask);
}

bool SpfHeader::IsValid()
{
	return strncmp(SpfIdentify, "SPFPKG", 6) == 0;
}

void SpfHeader::Read(PakStream in)
{
	in->read(SpfIdentify, 6);
	in->read((char*)&Version, sizeof(uint32_t));
	in->read(XorMask, 16);
}

void SpfHeader::Write(std::ostream& out) const
{
	out.write(SpfIdentify, 6);
	out.write((char*)&Version, sizeof(uint32_t));
	out.write(XorMask, 16);
}

size_t SpfHeader::Size()
{
	return 6 + 16 + sizeof(uint32_t);
}

void SpfMetadataHeader::Read(PakStream in)
{
	in->read((char*)&MetadataLen, sizeof(uint64_t));
	in->read((char*)&MetadataCount, sizeof(uint64_t));
}

void SpfMetadataHeader::Write(std::ostream& out) const
{
	out.write((char*)&MetadataLen, sizeof(uint64_t));
	out.write((char*)&MetadataCount, sizeof(uint64_t));
}

size_t SpfMetadataHeader::Size()
{
	return sizeof(uint64_t) * 2;
}

SpfMetadataValueKeyPair::SpfMetadataValueKeyPair()
{
	KeyLen = 0;
	Key = NULL;
	ValueLen = 0;
	Value = NULL;
}

SpfMetadataValueKeyPair::SpfMetadataValueKeyPair(const std::string& key, const std::string& value)
{
	KeyLen = key.size();
	Key = new char[key.size()];
	memcpy(Key, key.c_str(), key.size());

	ValueLen = value.size();
	Value = new char[value.size()];
	memcpy(Value, value.c_str(), value.size());
}

void SpfMetadataValueKeyPair::Read(PakStream in)
{
	in->read((char*)&KeyLen, sizeof(int32_t));
	Key = new char[KeyLen + 1];
	memset(Key, 0, KeyLen + 1);
	in->read(Key, KeyLen);
	in->read((char*)&ValueLen, sizeof(int32_t));
	Value = new char[ValueLen + 1];
	memset(Value, 0, ValueLen + 1);
	in->read(Value, ValueLen);
}

void SpfMetadataValueKeyPair::Write(std::ostream& out) const
{
	out.write((char*)&KeyLen, sizeof(int32_t));
	out.write(Key, KeyLen);
	out.write((char*)&ValueLen, sizeof(int32_t));
	out.write(Value, ValueLen);
}

size_t SpfMetadataValueKeyPair::Size()
{
	return KeyLen + ValueLen + sizeof(int32_t) * 2;
}

SpfMetadataValueKeyPair::~SpfMetadataValueKeyPair()
{
	if (Key) delete[] Key;
	if (Value) delete[] Value;
}

void SpfFileIndex::Read(PakStream in)
{
	in->read((char*)&FileMetaLen, sizeof(uint64_t));
	in->read((char*)&FileCount, sizeof(uint64_t));
}

void SpfFileIndex::Write(std::ostream& out) const
{
	out.write((char*)&FileMetaLen, sizeof(uint64_t));
	out.write((char*)&FileCount, sizeof(uint64_t));
}

size_t SpfFileIndex::Size()
{
	return sizeof(uint64_t) * 2;
}

SpfFileMetadata::SpfFileMetadata()
{
	Filename = 0;
}

SpfFileMetadata::SpfFileMetadata(const std::string& filename)
{
	FilenameLen = filename.size();
	Filename = (char*)malloc(filename.size());
	memcpy(Filename, filename.c_str(), filename.size());
}

void SpfFileMetadata::Read(PakStream in)
{
	in->read((char*)&FilenameLen, sizeof(uint8_t));
	Filename = new char[FilenameLen + 1];
	memset(Filename, 0, FilenameLen + 1);
	in->read(Filename, FilenameLen);
	in->read((char*)&ContentsLen, sizeof(uint64_t));
	in->read((char*)&ContentIndex, sizeof(uint64_t));
}

void SpfFileMetadata::Write(std::ostream& out) const
{
	out.write((char*)&FilenameLen, sizeof(uint8_t));
	out.write(Filename, FilenameLen);
	out.write((char*)&ContentsLen, sizeof(uint64_t));
	out.write((char*)&ContentIndex, sizeof(uint64_t));
}

size_t SpfFileMetadata::Size()
{
	return FilenameLen + sizeof(uint8_t) + sizeof(uint64_t) * 2;
}

SpfFileMetadata::~SpfFileMetadata()
{
	if (Filename) free(Filename);
}

SpfFileStream::SpfFileStream(const SpfFile& file, SpfFileMetadata* pFileMetadata)
{
	m_StreamPosition = 0;
	m_FileMetadata = pFileMetadata;
	m_PakStream = file.Stream;
	m_FileHeader = file.Header;
}

SpfFileStream::SpfFileStream(PakStream stream)
{
	m_PakStream = stream;
	m_StreamPosition = 0;
	m_FileMetadata = NULL;
	m_FileHeader = {};
}

size_t SpfFileStream::read(void* buffer, size_t size)
{
	if (!m_FileMetadata)
	{
		return m_PakStream->read(buffer, size);
	}

	char* buff = (char*)buffer;

	size_t filePointer = m_StreamPosition + m_FileMetadata->ContentIndex;
	size_t g = m_PakStream->tellg();
	if (g != filePointer)
	{
		m_PakStream->seekg(filePointer);
	}
	size_t readed = m_PakStream->read(buffer, size);

	for (int i = 0; i < readed; i++)
	{
		size_t index = i + m_StreamPosition;
		char xorKey = m_FileHeader.XorMask[index % 16] ^ (index & 0xFF);
		buff[i] ^= xorKey;
	}

	m_StreamPosition += readed;
	return readed;
}

void SpfFileStream::seekg(size_t pos, int mode)
{
	if (!m_FileMetadata)
	{
		m_PakStream->seekg(pos, mode);
		return;
	}
	if (mode == SEEK_CUR)
	{
		m_StreamPosition += pos;
		return;
	}
	if (mode == SEEK_END)
	{
		return;
	}
	m_StreamPosition = pos;
}
void SpfFileStream::ignore(size_t size)
{
	m_PakStream->ignore(size);
	m_StreamPosition += size;
}

size_t SpfFileStream::tellg()
{
	if (!m_FileMetadata)
	{
		return m_PakStream->tellg();
	}
	return m_StreamPosition;
}

bool SpfFileStream::eof()
{
	return m_PakStream->eof();
}

bool SpfFileStream::is_open()
{
	return m_PakStream->is_open();
}

bool SpfFileStream::is_self_contained()
{
	return !m_FileMetadata;
}
