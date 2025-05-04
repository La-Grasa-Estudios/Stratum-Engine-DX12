#include "ZVFS.h"

#include <iostream>
#include <filesystem>

#include "Core/Logger.h"

#include <zlib/izlibstream.h>

using namespace ENGINE_NAMESPACE;

#define MAXIMUM_CACHE_SIZE_IN_MB 64
constexpr size_t CACHE_SIZE = MAXIMUM_CACHE_SIZE_IN_MB * 1024 * 1024;

/*
struct cached_file_t
{
    RefBinaryStream stream;
    uint64_t time;
};

std::unordered_map<std::string, cached_file_t> g_FileCache;
size_t g_FileCacheSize = 0;

RefBinaryStream PutInCache(const char* file, RefBinaryStream stream)
{

    if (stream->Size() > CACHE_SIZE / 2)
    {
        return stream;
    }

    while (g_FileCacheSize + stream->Size() > CACHE_SIZE)
    {

        std::string oldestEntry = "";
        cached_file_t* oldestFile = NULL;
        uint64_t oldestTime = 0;

        for (auto& pair : g_FileCache)
        {
            cached_file_t& f = pair.second;
            if (f.time > oldestTime)
            {
                oldestTime = f.time;
                oldestEntry = pair.first;
                oldestFile = &f;
            }
        }

        if (!oldestEntry.empty())
        {
            g_FileCacheSize -= oldestFile->stream->Size();
            g_FileCache.erase(oldestEntry);
        }
    }

    g_FileCacheSize += stream->Size();

    std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
    auto since_epoch = begin.time_since_epoch();
    

    cached_file_t cachedFile;
    cachedFile.stream = stream;
    cachedFile.time = (std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count());

    g_FileCache[file] = cachedFile;

    //Z_INFO("Cache size is now: {}KB", g_FileCacheSize / 1024);

    return stream;

}
*/
bool IsSupportedBC7ImageFile(std::filesystem::path path) {
    std::string ex = path.extension().string();
    return (strncmp(ex.c_str(), ".png", 3) == 0 || strncmp(ex.c_str(), ".tga", 3) == 0 || strncmp(ex.c_str(), ".jpg", 3) == 0 ||
        strncmp(ex.c_str(), ".bmp", 3) == 0 || strncmp(ex.c_str(), ".hdr", 3) == 0);
}

void ZVFS::Init()
{
    if (m_Instance) delete m_Instance;
	m_Instance = new ZVFS();
    //g_FileCache = {};
}

void ZVFS::Mount(std::string directory)
{
    Mount(directory, false);
}

void ENGINE_NAMESPACE::ZVFS::Mount(std::string directory, bool root)
{

    if (!std::filesystem::exists(directory))
    {
        Z_ERROR("Failed to mount directory {}", directory);
        return;
    }

    ZVFS* filesystem = m_Instance;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        std::filesystem::path path = entry.path();
        if (path.has_extension()) {
            std::string extension = path.extension().string();
            if (extension == ".zpk") {
                AssetPack* pack = AssetPack::MountFile(path.string());
                if (pack) {
                    filesystem->packs.push_back(pack);
                }
            }
            else {
                std::string p = path.string();
                for (int i = 0; i < p.size(); i++) {
                    if (p[i] == '\\') {
                        p[i] = '/';
                    }
                }
                ZVFSFile file;

                std::transform(p.begin(), p.end(), p.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                file.globalResourceData = NULL;
                file.name = root ? p.c_str() + (directory.size() + 1) : p;
                file.path = path.string();
                filesystem->files[file.name] = file;
            }
        }
    }
    for (auto pack : filesystem->packs)
    {
        for (int i = 0; i < pack->entries.size(); i++)
        {
            ZVFSPackFile file{};

            file.pEntry = &pack->entries[i];
            file.pPack = pack;

            filesystem->pfiles[file.pEntry->Path] = file;
        }
    }

}

void ZVFS::MountFile(std::string path)
{
    auto file = AssetPack::MountFile(path);
    if (file)
    {
        ZVFS* filesystem = m_Instance;
        filesystem->packs.push_back(file);
    }
}

void ZVFS::MountEmbeddedFile(const char* filePath, int resourceId, const char* resourceType)
{
}

RefBinaryStream ZVFS::GetFile(const char* file)
{
    /*
    if (g_FileCache.contains(file))
    {
        auto stream = std::stringstream(g_FileCache[file].stream->Str());
        return CreateRef<BinaryStream>(stream);
    }
    */

    std::string sfile = file;

    if (sfile.starts_with("/"))
    {
        sfile = sfile.substr(1);
    }
    if (sfile.starts_with("./"))
    {
        sfile = sfile.substr(2);
    }

    if (IsSupportedBC7ImageFile(sfile)) {
        std::filesystem::path p = sfile;
        sfile = p.replace_extension(".bc7").string();
    }

    for (int i = 0; i < sfile.size(); i++) {
        if (sfile[i] == '\\') {
            sfile[i] = '/';
        }
    }

    if (m_Instance->pfiles.contains(sfile))
    {
        ZVFSPackFile zfile = m_Instance->pfiles[sfile];
        AssetPack* pack = zfile.pPack;
        if (g_VFSDebug)
        {
            Z_INFO("[VFS] Reading {} from {} pak file", sfile, (const char*)pack->header.PakName);
        }
        std::stringstream stream = pack->GetFile(*zfile.pEntry);
        return CreateRef<BinaryStream>(stream);
        //return PutInCache(file, CreateRef<BinaryStream>(stream));
    }
    
    sfile = file;

    for (int i = 0; i < sfile.size(); i++) {
        if (sfile[i] == '\\') {
            sfile[i] = '/';
        }
    }

    std::transform(sfile.begin(), sfile.end(), sfile.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (m_Instance->files.contains(sfile))
    {
        ZVFSFile& zfile = m_Instance->files[sfile];

        std::stringstream stream("");

        if (zfile.globalResourceData)
        {
            stream.write(zfile.globalResourceData, zfile.globalResourceSize);
        }
        else
        {
            std::ifstream in(zfile.path, std::ios::binary);
            stream << in.rdbuf();
            in.clear();
            in.seekg(0, std::ios::beg);
        }
        
        if (g_VFSDebug)
        {
            Z_INFO("[VFS] Reading {} from file registry", sfile, zfile.path);
        }
        return CreateRef<BinaryStream>(stream);
        //return PutInCache(file, CreateRef<BinaryStream>(stream));
    }

    std::ifstream in(file, std::ios::binary);

    if (in.is_open()) {
        std::stringstream sstream;
        sstream << in.rdbuf();
        if (g_VFSDebug)
        {
            Z_INFO("[VFS] Reading {} from fstream", file);
        }
        return CreateRef<BinaryStream>(sstream);
        //return PutInCache(file, CreateRef<BinaryStream>(sstream));
    }

    std::stringstream null("");
	return CreateRef<BinaryStream>(null);
}

PakStream ZVFS::GetFileStream(const char* file)
{

    std::string sfile = file;

    if (sfile.starts_with("/"))
    {
        sfile = sfile.substr(1);
    }
    if (sfile.starts_with("./"))
    {
        sfile = sfile.substr(2);
    }

    if (IsSupportedBC7ImageFile(sfile)) {
        std::filesystem::path p = sfile;
        sfile = p.replace_extension(".bc7").string();
    }

    if (m_Instance->pfiles.contains(sfile))
    {
        ZVFSPackFile file = m_Instance->pfiles[sfile];
        AssetPack* pack = file.pPack;
        if (g_VFSDebug)
        {
            Z_INFO("[VFS] Creating stream for file {} in {} pak file", sfile, (const char*)pack->header.PakName);
        }
        return CreateRef<PakStreamImpl>(*file.pEntry, pack);
    }

    sfile = file;

    for (int i = 0; i < sfile.size(); i++) {
        if (sfile[i] == '\\') {
            sfile[i] = '/';
        }
    }

    std::transform(sfile.begin(), sfile.end(), sfile.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (m_Instance->files.contains(sfile))
    {
        ZVFSFile& zfile = m_Instance->files[sfile];
        if (g_VFSDebug) Z_INFO("[VFS] Creating stream for file registry {}", sfile);

        if (zfile.globalResourceData)
        {

            if (g_VFSDebug) Z_INFO("[VFS] File stream is from embedded resource: {}", sfile);

            Ref<std::stringstream> in = CreateRef<std::stringstream>();
            in->write(zfile.globalResourceData, zfile.globalResourceSize);
            return CreateRef<PakStreamImpl>(in);
        }

        Ref<std::istream> in = CreateRef<std::ifstream>(zfile.path, std::ios::binary);
        return CreateRef<PakStreamImpl>(in);
    }

    Ref<std::ifstream> in = CreateRef<std::ifstream>(file, std::ios::binary);

    if (in->is_open()) {
        if (g_VFSDebug) Z_INFO("[VFS] Creating stream for file {}", file);
        return CreateRef<PakStreamImpl>(in);
    }

    return NULL;
}

bool ZVFS::Exists(const char* file)
{
    std::string sfile = file;

    if (sfile.starts_with("/"))
    {
        sfile = sfile.substr(1);
    }
    if (sfile.starts_with("./"))
    {
        sfile = sfile.substr(2);
    }

    if (g_VFSDebug)
    {
        Z_INFO("[VFS] Checking if file exists: {}", file);
    }

    if (IsSupportedBC7ImageFile(sfile)) {
        std::filesystem::path p = sfile;
        sfile = p.replace_extension(".bc7").string();
    }

    for (int i = 0; i < sfile.size(); i++) {
        if (sfile[i] == '\\') {
            sfile[i] = '/';
        }
    }

    if (m_Instance->pfiles.contains(sfile))
    {
        if (g_VFSDebug) 
        {
            ZVFSPackFile file = m_Instance->pfiles[sfile];
            AssetPack* pack = file.pPack;
            Z_INFO("[VFS] Found {} in pak {}", sfile, (char*)pack->header.PakName);
        }
        return true;
    }

    sfile = file;

    for (int i = 0; i < sfile.size(); i++) {
        if (sfile[i] == '\\') {
            sfile[i] = '/';
        }
    }

    std::transform(sfile.begin(), sfile.end(), sfile.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (m_Instance->files.contains(sfile))
    {
        ZVFSFile& zfile = m_Instance->files[sfile];
        if (g_VFSDebug) Z_INFO("[VFS] Found {} in file registry", zfile.name);
        return true;
    }

    if (g_VFSDebug) Z_INFO("[VFS] file {} not found in VFS registry forwarding call to the OS filesystem", file);

    return std::filesystem::exists(file);
}

std::vector<std::string> ZVFS::GetAllOf(const char* extension)
{
    std::vector<std::string> files;

    for (auto& file : m_Instance->pfiles)
    {
        const std::string& filename = file.first;

        if (filename.ends_with(extension))
        {
            files.push_back(filename);
        }
    }

    for (auto& file : m_Instance->files)
    {
        const std::string& filename = file.first;

        if (filename.ends_with(extension))
        {
            files.push_back(filename);
        }
    }

    return files;
}

PakStreamImpl::PakStreamImpl(AssetFileEntry& entry, AssetPack* in)
{
    this->m_Pack = in;
    this->m_Entry = entry;

    m_ZlibStream = 0;

    if (m_Entry.IsCompressed) {
        this->m_Pack->semaphore.acquire();
        this->m_Pack->stream->clear();
        this->m_Pack->stream->seekg(m_Entry.Offset + 10);
        m_ZlibStream = new zlib::izlibstream(*m_Pack->stream.get());
        this->m_Pack->semaphore.release();
    }

}

PakStreamImpl::PakStreamImpl(Ref<std::istream> in)
{
    this->stream = in;
}

PakStreamImpl::PakStreamImpl()
{
    this->stream = NULL;
}

size_t PakStreamImpl::read(void* buffer, size_t size)
{

    char* cb = (char*)buffer;

    if (m_eof) {
        return 0;
    }

    if (stream) {
        stream->read(cb, size);
        if (stream->eof())
        {
            m_eof = true;
        }
        return stream->gcount();
    }

    int remaining = m_Entry.Size - gpointer;
    if (remaining < size) {
        m_eof = true;
        size = remaining;
    }

    this->m_Pack->semaphore.acquire();

    if (m_ZlibStream) {
        this->m_Pack->stream->clear();
        this->m_Pack->stream->seekg(m_Entry.Offset + 10);
        zlib::izlibstream in (*m_Pack->stream, 1024);
        in.ignore(gpointer);
        in.read(cb, size);
        in.close();
    }
    else {
        this->m_Pack->stream->clear();
        this->m_Pack->stream->seekg(m_Entry.Offset + 10 + gpointer);
        this->m_Pack->stream->read(cb, size);
    }

    size_t amountRead = this->m_Pack->stream->gcount();

    this->m_Pack->semaphore.release();

    int ptr = gpointer;
    gpointer += size;

    for (int i = 0; i < size; i++) {
        size_t index = (i + ptr) % AssetPack::xorBlockSize;
        char key = AssetPack::blockKeys[index];
        if (this->m_Pack->header.Version < ASSET_PACKER_VER_BLOCK_ENCRYPTION)
        {
            key = 18;
        }
        char value = cb[i];
        value = value ^ key;
        cb[i] = value;
    }

    return amountRead;
}

void PakStreamImpl::seekg(size_t pos, int mode)
{
    m_eof = false;
    if (stream)
    {
        if (stream->fail() || stream->eof())
        {
            stream->clear();
        }
        if (mode == SEEK_CUR)
        {
            stream->seekg(pos, std::ios::_Seekcur);
        }
        if (mode == SEEK_END)
        {
            stream->seekg(pos, std::ios::_Seekend);
            return;
        }
        stream->seekg(pos, std::ios::_Seekbeg);
    }
    if (mode == SEEK_CUR)
    {
        gpointer += pos;
        return;
    }
    if (mode == SEEK_END)
    {
        gpointer = m_Entry.Size + pos;
        return;
    }
    gpointer = pos;
}

void PakStreamImpl::ignore(size_t size)
{
    int remaining = m_Entry.Size - gpointer;
    if (remaining < size) {
        m_eof = true;
        size = remaining;
    }
    gpointer += size;
}

size_t PakStreamImpl::tellg()
{
    if (stream)
    {
        return stream->tellg();
    }
    return gpointer;
}

bool PakStreamImpl::eof()
{
    return m_eof;
}

bool PakStreamImpl::is_open()
{
    return stream || m_Pack;
}

bool PakStreamImpl::is_encrypted()
{
    return m_Pack && m_Pack->header.Version >= ASSET_PACKER_VER_BLOCK_ENCRYPTION;
}

void PakStreamImpl::AcquireLock()
{
    if (m_Pack)
    {
        this->m_Pack->semaphore.acquire();
    }
}

void PakStreamImpl::ReleaseLock()
{
    if (m_Pack)
    {
        this->m_Pack->semaphore.release();
    }
}

std::istream* PakStreamImpl::GetStream()
{
    if (m_Pack)
    {
        this->m_Pack->stream->seekg(m_Entry.Offset + 10 + gpointer);
    }
    return stream != NULL ? stream.get() : m_Pack->stream.get();
}

PakStreamImpl::~PakStreamImpl()
{
    if (m_ZlibStream) delete (zlib::izlibstream*)m_ZlibStream;
}
