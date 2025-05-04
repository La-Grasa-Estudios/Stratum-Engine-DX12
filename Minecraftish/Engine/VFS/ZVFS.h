#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>

#include "Core/Ref.h"
#include "AssetPack.h"
#include "znmsp.h"

BEGIN_ENGINE

struct BinaryStream {

	BinaryStream(std::stringstream& stream) {
		m_Data = stream.str();
		m_Stream = std::istringstream(m_Data);
	}

	template<typename T>
	T* As() {
		return (T*)m_Data.c_str();
	}

	size_t Size() {
		return m_Data.size();
	}

	std::string Str() {
		return m_Stream.str();
	}

	std::istream* Stream() {
		return &m_Stream;
	}

private:

	std::string m_Data;
	std::istringstream m_Stream;

};

class PakStreamImpl {

public:

	PakStreamImpl(AssetFileEntry& entry, AssetPack* in);
	PakStreamImpl(Ref<std::istream> in);
	PakStreamImpl();

	size_t read(void* buffer, size_t size);
	void seekg(size_t pos, int mode = 0);
	void ignore(size_t size);
	size_t tellg();
	bool eof();
	bool is_open();
	bool is_encrypted();

	void AcquireLock();
	void ReleaseLock();

	std::istream* GetStream();

	~PakStreamImpl();

private:

	bool m_eof = false;
	int gpointer = 0;

	AssetFileEntry m_Entry;
	AssetPack* m_Pack = NULL;
	void* m_ZlibStream = NULL;
	Ref<std::istream> stream;

};

using RefBinaryStream = Ref<BinaryStream>;
using PakStream = Ref<PakStreamImpl>;

struct ZVFSFile {
	char* globalResourceData;
	size_t globalResourceSize;
	std::ifstream* in;
	std::string name;
	std::string path;
};

struct ZVFSPackFile
{
	AssetFileEntry* pEntry;
	AssetPack* pPack;
};

class ZVFS
{
public:

	DLLEXPORT inline static bool g_VFSDebug = false;

	DLLEXPORT static void Init();

	DLLEXPORT static void Mount(std::string directory);
	DLLEXPORT static void Mount(std::string directory, bool root);
	DLLEXPORT static void MountFile(std::string path);
	DLLEXPORT static void MountEmbeddedFile(const char* filePath, int resourceId, const char* resourceType);

	DLLEXPORT static RefBinaryStream GetFile(const char* file);
	DLLEXPORT static PakStream GetFileStream(const char* file);

	DLLEXPORT static bool Exists(const char* file);

	DLLEXPORT static std::vector<std::string> GetAllOf(const char* extension);

private:

	inline static ZVFS* m_Instance = NULL;

	std::vector<AssetPack*> packs;
	std::unordered_map<std::string, ZVFSFile> files;
	std::unordered_map<std::string, ZVFSPackFile> pfiles;

};

END_ENGINE

