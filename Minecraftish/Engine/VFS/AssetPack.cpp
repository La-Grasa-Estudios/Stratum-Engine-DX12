#include "AssetPack.h"

#include <filesystem>
#include <iostream>

#include <zlib/ozlibstream.h>
#include <zlib/izlibstream.h>

#include "Core/JobManager.h"
#include "Core/Logger.h"

#include <stb_image.h>
#include "bc7/Bc7Compress.h"
#include "base64.hpp"

#include <format>

#define BC7_MAGICK 0x375A4243;

using namespace ENGINE_NAMESPACE;

std::int32_t readInt(std::ifstream& in, int len)
{
    char buffer[4];
    in.read(buffer, 4);
    std::int32_t a = 0;
    if (std::endian::native == std::endian::little)
        std::memcpy(&a, buffer, len);
    else
        for (std::size_t i = 0; i < len; ++i)
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}

inline void writeInt(uint32 i, std::ofstream& stream) {
    stream.write((char*)&i, sizeof(uint32));
}
inline void writeShort(uint16 i, std::ofstream& stream) {
    stream.write((char*)&i, sizeof(uint16));
}
inline void writeByte(uint8 i, std::ofstream& stream) {
    stream.write((char*)&i, sizeof(uint8));
}

inline void writeUTF(const char* str, std::ofstream& stream) {
    int i = 0;
    while (str[i] != 0x00) {
        stream << str[i++];
    }
    stream << (char)0;
}

inline std::string readUTF(std::ifstream& stream) {
    char buffer[8];
    std::string str;
    while (stream.read(buffer, 1)) {
        if (buffer[0] == (char)0) {
            break;
        }
        str += buffer[0];
    }
    return str;
}

static std::string EncryptString(std::string str, char key) {

    char buffer[1];
    const char* strbuffer = str.c_str();

    std::stringstream strs;

    for (int i = 0; i < str.size(); i++) {
        buffer[0] = strbuffer[i] ^ key;
        strs.write(buffer, 1);
    }

    return strs.str();

}

static std::string DecryptString(std::string str, char key) {

    char buffer[1];
    const char* strbuffer = str.c_str();

    std::stringstream strs;

    for (int i = 0; i < str.size(); i++) {
        buffer[0] = strbuffer[i] ^ key;
        strs.write(buffer, 1);
    }

    return strs.str();

}

char* read_file(std::string path, unsigned int* size) {
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();
    file.seekg(0, std::ios_base::beg);

    char* block = new char[length];

    file.read(block, length);

    *size = (unsigned int)length;

    return block;
}

struct AssetPackerEntry {
    AssetFileEntry entry;
    char* data;
};

static bool IsSupportedCompressableImageFile(std::filesystem::path path) {
    std::string ex = path.extension().string();
    return (strncmp(ex.c_str(), ".png", 4) == 0 || strncmp(ex.c_str(), ".tga", 4) == 0 || strncmp(ex.c_str(), ".jpg", 4) == 0 ||
        strncmp(ex.c_str(), ".bmp", 4) == 0 || strncmp(ex.c_str(), ".hdr", 4) == 0);
}

static bool IsSupportedCompressableFile(std::filesystem::path path) {
    std::string ex = path.extension().string();
    if (ex.ends_with("mp4"))
    {
        return false;
    }
    if (ex.ends_with("mp3"))
    {
        return false;
    }
    if (ex.ends_with("wav"))
    {
        return false;
    }
    if (ex.ends_with("ctex"))
    {
        return false;
    }
    if (ex.ends_with("webm"))
    {
        return false;
    }
    if (ex.ends_with("brstm"))
    {
        return false;
    }
    if (ex.ends_with("ctex"))
    {
        return false;
    }
    return !(strncmp(ex.c_str(), ".mp4", 4) == 0 || strncmp(ex.c_str(), ".webm", 5) == 0 || strncmp(ex.c_str(), ".brstm", 6) == 0) || !(strncmp(ex.c_str(), ".ctex", 5) == 0);
}

static void ReadFileEntryFromPak(std::ifstream& in, const AssetFileHeader& header, AssetFileEntry* entry)
{
    std::string name = readUTF(in);
    if (header.Version >= ASSET_PACKER_VER_B64) name = base64::from_base64(name);
    if (header.Version >= ASSET_PACKER_VER_B64_XOR) name = DecryptString(name, 13);
    bool compressed = false;
    in.read((char*)&compressed, 1);
    uint32 size = readInt(in, 4);
    uint64 offset = readInt(in, sizeof(uint64));
    entry->IsCompressed = compressed;
    entry->Offset = offset;
    entry->Size = size;
    entry->Path = new char[name.size() + 1];
    memcpy(entry->Path, name.c_str(), name.size() + 1);
    for (int i = 0; i < name.size(); i++) {
        //entry.Path[i] = entry.Path[i] ^ 11;
        if (entry->Path[i] == '\\') {
            entry->Path[i] = '/';
        }
    }
}

static uint8_t* UnpackImage(uint8_t* buffer, uint32_t width, uint32_t height, uint32_t channels, bool* isTransparent) {

    uint8_t* newBuffer = buffer;

    uint32_t texelSize = 4;

    if (channels != 4) {
        if (channels == 1) texelSize = 1;
        if (channels == 2) texelSize = 2;

        newBuffer = new uint8_t[width * height * texelSize];
    }

    if (channels <= 2)
    {
        memcpy(newBuffer, buffer, width * height * channels);
        return newBuffer;
    }

    uint32_t cSize = width * height * channels;
    uint32_t iter = cSize / channels;

    for (uint32_t i = 0; i < iter && channels > 2; i++) {

        if (channels == 4) {
            uint8_t r = buffer[i * 4 + 0];
            buffer[i * 4 + 0] = buffer[i * 4 + 2];
            buffer[i * 4 + 2] = r;
            uint8_t alpha = buffer[i * 4 + 3];
            if (alpha <= 250) {
                *isTransparent = true;
            }
        }

        if (channels == 3) {

            unsigned char r = ((unsigned char*)buffer)[i * 3 + 0];
            unsigned char g = ((unsigned char*)buffer)[i * 3 + 1];
            unsigned char b = ((unsigned char*)buffer)[i * 3 + 2];

            newBuffer[i * 4 + 0] = b;
            newBuffer[i * 4 + 1] = g;
            newBuffer[i * 4 + 2] = r;
            newBuffer[i * 4 + 3] = (unsigned char)255;

        }

    }

    if (channels != 4) {
        return newBuffer;
    }
    else {
        return buffer;
    }
}

static uint8_t* GenerateLODImage(uint8_t* brgaBuff, uint32_t width, uint32_t height, uint32_t nrChannels, uint32_t* pLodFactor) {
    
    int lodReduction = 2;

    while (width / lodReduction > lodReduction && height / lodReduction > lodReduction && lodReduction < 4) {
        lodReduction += 1;
    }

    uint32_t texelSize = 4;

    if (nrChannels == 1) texelSize = 1;
    if (nrChannels == 2) texelSize = 2;

    uint8_t* buffer = new uint8_t[(width * height * texelSize) / lodReduction];

    *pLodFactor = lodReduction;

    if (width > lodReduction && height > lodReduction) {
        for (int x = 0; x < width / lodReduction; x++) {
            for (int y = 0; y < height / lodReduction; y++) {

                uint32_t avgr = 0;
                uint32_t avgg = 0;
                uint32_t avgb = 0;
                uint32_t avga = 0;

                uint32_t div = 0;

                for (int ix = 0; ix < lodReduction; ix++) {
                    for (int iy = 0; iy < lodReduction; iy++) {

                        div++;

                        int index = (x * lodReduction + ix) + (y * lodReduction + iy) * width;

                        if (nrChannels == 1)
                        {
                            avgr = brgaBuff[index];
                            continue;
                        }
                        if (nrChannels == 2)
                        {
                            avgr = brgaBuff[index * 2 + 0];
                            avgg = brgaBuff[index * 2 + 1];
                            continue;
                        }

                        avgr += brgaBuff[index * 4 + 0];
                        avgg += brgaBuff[index * 4 + 1];
                        avgb += brgaBuff[index * 4 + 2];
                        avga += brgaBuff[index * 4 + 3];

                    }
                }

                int index = x + y * (width / lodReduction);

                if (nrChannels == 1)
                {
                    buffer[index] = avgr;;
                    continue;
                }
                if (nrChannels == 2)
                {
                    buffer[index * 2 + 0] = avgr;
                    buffer[index * 2 + 1] = avgg;
                    continue;
                }

                buffer[index * 4 + 0] = avgr / div;
                buffer[index * 4 + 1] = avgg / div;
                buffer[index * 4 + 2] = avgb / div;
                buffer[index * 4 + 3] = avga / div;

            }
        }
    }

    return buffer;

}

static uint8_t* CompressImageFile(uint8_t* buffer, uint32_t size, uint32_t* pNewSize) {

    uint32_t header[9]{};

    int width, height;
    int nrChannels;

    uint8_t* data = stbi_load_from_memory(buffer, size, &width, &height, &nrChannels, 0);

    uint32_t lodFactor = 0;

    bool transparent = false;

    uint8_t* brga_src = UnpackImage(data, width, height, nrChannels, &transparent);
    uint8_t* lod_brga_src = GenerateLODImage(brga_src, width, height, nrChannels, &lodFactor);

    header[0] = BC7_MAGICK;
    header[1] = width;
    header[2] = height;
    header[3] = nrChannels;
    header[4] = lodFactor;
    header[5] = 1;
    header[8] = transparent;

    Z_INFO("Image desc Width: {}, Height: {}, Channels: {}, LodFactor: {}, Transparent: {}", width, height, nrChannels, lodFactor, transparent)

    if (!(width % 4 == 0 && height % 4 == 0) || (nrChannels <= 2)) {
        // Non power of 4 or is less than 3 channels, so leave it uncompressed

        Z_INFO("Skipping {} by {} image since bc7 does not support non power of 4 textures", width, height);

        header[5] = 0;

        uint32_t brga_siz = width * height * nrChannels;
        uint32_t brga_lod_siz = brga_siz / lodFactor;
        uint32_t buffSize = sizeof(header) + brga_lod_siz + brga_siz;

        uint8_t* finalBuffer = new uint8_t[buffSize];

        memcpy(finalBuffer, header, sizeof(header));
        memcpy(finalBuffer + sizeof(header), brga_src, brga_siz);
        memcpy(finalBuffer + sizeof(header) + brga_siz, lod_brga_src, brga_lod_siz);

        delete[] brga_src;
        delete[] lod_brga_src;
        if (nrChannels != 4) stbi_image_free(data);

        *pNewSize = buffSize;
        return finalBuffer;
    }

    BC7::Bc7Result bc7 = BC7::Compress::Bc7Compress(brga_src, width, height);
    BC7::Bc7Result lodbc7 = BC7::Compress::Bc7Compress(lod_brga_src, width / lodFactor, height / lodFactor);

    delete[] brga_src;
    delete[] lod_brga_src;
    if (nrChannels != 4) stbi_image_free(data);

    uint8_t* finalBuffer = new uint8_t[sizeof(header) + bc7.size + lodbc7.size];

    header[6] = bc7.size;
    header[7] = lodbc7.size;

    memcpy(finalBuffer, header, sizeof(header));
    memcpy(finalBuffer + sizeof(header), bc7.buffer, bc7.size);
    memcpy(finalBuffer + sizeof(header) + bc7.size, lodbc7.buffer, lodbc7.size);

    delete[] bc7.buffer;
    delete[] lodbc7.buffer;

    *pNewSize = sizeof(header) + bc7.size + lodbc7.size;
    return finalBuffer;

}

void AssetPacker::Pack(std::string directory, std::string out, bool dirIsRoot, const char** pIgnore, int32_t nbIgnore)
{

    constexpr size_t maxPackSize = 64 * 1024 * 1024;

    std::vector<AssetPackerEntry> entries;
    auto pEntries = &entries;
    bool compress = Compress;

    std::binary_semaphore semaphore(1);
    auto pSemaphore = &semaphore;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        std::filesystem::path fpath = entry.path();
        if (fpath.has_extension()) {

            std::string spath = fpath.string();

            bool shouldIgnore = false;

            for (int i = 0; i < nbIgnore && !shouldIgnore; i++)
            {
                std::string ignore = pIgnore[i];

                if (spath.find(ignore) != std::string::npos)
                {
                    shouldIgnore = true;
                }
            }

            if (!shouldIgnore)
            {
                JobManager::Execute([pEntries, directory, fpath, compress, pSemaphore, shouldIgnore]() {

                    std::filesystem::path path = fpath;
                    uint32 size = 0;
                    char* data = read_file(path.string(), &size);

                    if (IsSupportedCompressableImageFile(path)) {
                        char* image = reinterpret_cast<char*>(CompressImageFile(reinterpret_cast<uint8_t*>(data), size, &size));
                        delete[] data;
                        data = image;
                        path.replace_extension(".bc7");
                    }

                    bool supportsCompression = true;

                    if (!IsSupportedCompressableFile(path)) {
                        supportsCompression = false;
                    }

                    AssetPackerEntry file;
                    AssetFileEntry& entry = file.entry;

                    std::filesystem::path dir = directory;
                    std::string dirName = dir.parent_path().string();

                    std::string out = directory;
                    out.append(".dir");
                    std::filesystem::path outBaseDir = out;
                    outBaseDir = outBaseDir.filename().stem();

                    std::string spath = outBaseDir.string().append("/").append(path.lexically_relative(directory).string());

                    AssetPack::BlockXorEncrypt(data, size);

                    if (compress && supportsCompression) {
                        std::stringstream zlibstream;
                        zlib::ozlibstream zout(zlibstream, 9);
                        zout.write(data, size);
                        zout.close();
                        delete[] data;
                        std::string sdata = zlibstream.str();
                        data = new char[sizeof(char) * sdata.size()];
                        memcpy(data, sdata.c_str(), sdata.size());
                        entry.CompressedSize = (uint32_t)sdata.size();
                    }
                    else {
                        entry.CompressedSize = size;
                    }

                    file.data = data;

                    for (int i = 0; i < spath.size(); i++) {
                        if (spath[i] == '\\') {
                            spath[i] = '/';
                        }
                    }

                    entry.Path = new char[sizeof(char) * spath.size() + 1];
                    memset(entry.Path, 0, sizeof(char) * spath.size() + 1);
                    entry.PathSize = (uint16)(sizeof(char) * spath.size() + 1);
                    memcpy(entry.Path, spath.c_str(), sizeof(char) * spath.size());

                    entry.Size = size;
                    entry.IsCompressed = compress && supportsCompression;

                    pSemaphore->acquire();
                    pEntries->push_back(file);
                    pSemaphore->release();
                    });
            }

        }
        else {
            Z_INFO("Scanning {} for files", fpath.string());
        }
    }

    JobManager::Wait();

    for (int i = 0; i < entries.size() && dirIsRoot; i++)
    {
        AssetPackerEntry& a = entries[i];
        std::string path = a.entry.Path;
        path = path.substr(directory.size() + (directory.ends_with('/') ? 0 : 1));
        delete[] a.entry.Path;
        a.entry.Path = new char[sizeof(char) * path.size() + 1];
        memset(a.entry.Path, 0, sizeof(char)* path.size() + 1);
        a.entry.PathSize = (uint16)(sizeof(char) * path.size() + 1);
        memcpy(a.entry.Path, path.c_str(), sizeof(char)* path.size());

    }

    for (int i = 0; i < entries.size(); i++)
    {
        AssetPackerEntry& a = entries[i];
        std::string p = a.entry.Path;

        p = EncryptString(p, 13);

        std::string path = base64::to_base64(p);
        delete[] a.entry.Path;
        a.entry.Path = new char[sizeof(char) * path.size() + 1];
        memset(a.entry.Path, 0, sizeof(char)* path.size() + 1);
        a.entry.PathSize = (uint16)(sizeof(char) * path.size() + 1);
        memcpy(a.entry.Path, path.c_str(), sizeof(char)* path.size());
    }

    AssetFileHeader header;

    header.NbEntries = (uint32)entries.size();
    header.ContentVersion = "1.0.0.0";
    
    memset(header.PakName, 0, sizeof(header.PakName));
    memcpy(header.PakName, out.c_str(), out.size());

    uint32_t nbFiles = 1;
    size_t entryIdx = 0;
    uint32_t fileIndex = 0;

    while (nbFiles > 0) {

        nbFiles--;

        std::string o{ std::format("{}_zpk_{:03d}.zpk", out, fileIndex++) };

        uint64 maxIdx = entryIdx;

        uint64 ss = 0;

        uint32_t nbEntries = 0;

        for (size_t i = entryIdx; i < entries.size(); i++) {
            AssetPackerEntry& entry = entries[i];
            ss += entry.entry.CompressedSize;
            maxIdx = i + 1;
            nbEntries++;
            if (ss > maxPackSize) {
                nbFiles++;
                break;
            }
        }

        header.NbEntries = nbEntries;

        if (nbEntries == 0) {
            break;
        }

        std::ofstream fout(o, std::ios::binary);

        writeUTF((const char*)header.identifier, fout);
        writeShort(header.Version, fout);
        writeUTF(header.ContentVersion.c_str(), fout);
        writeUTF((const char*)header.PakName, fout);
        writeInt(header.NbEntries, fout);

        uint64 offset = (uint64)fout.tellp();

        for (size_t i = entryIdx; i < maxIdx; i++) {
            AssetPackerEntry& entry = entries[i];
            uint64 size = 0;
            size += entry.entry.PathSize;
            size += sizeof(uint32) + sizeof(uint8) + sizeof(uint64);
            offset += size;
        }

        for (size_t i = entryIdx; i < maxIdx; i++) {
            AssetPackerEntry& entry = entries[i];
            entry.entry.Offset = offset;
            offset += entry.entry.CompressedSize + 10;
        }

        for (size_t i = entryIdx; i < maxIdx; i++) {
            AssetPackerEntry& file = entries[i];
            writeUTF((const char*)file.entry.Path, fout);
            writeByte((char)file.entry.IsCompressed, fout);
            writeInt(file.entry.Size, fout);
            fout.write((char*)&file.entry.Offset, sizeof(uint64));
        }

        for (size_t i = entryIdx; i < maxIdx; i++) {
            AssetPackerEntry& file = entries[i];
            writeUTF("FileEntry", fout);
            fout.write(file.data, file.entry.CompressedSize);
        }

        entryIdx = maxIdx;

        fout.close();
    }

    for (auto& file : entries) {

        delete[] file.entry.Path;
        delete[] file.data;

    }

}

AssetPack* AssetPack::MountFile(std::string file)
{
    AssetPack* pack = new AssetPack();
    AssetFileHeader& header = pack->header;

    pack->stream = std::make_unique<std::ifstream>(file, std::ios::binary);

    std::ifstream& in = *pack->stream.get();

    if (!in.is_open())
    {
        delete pack;
        return NULL;
    }

    char buffer[128];
    const char* headerIdentifier = (const char*)header.identifier;

    if (!in.read(buffer, sizeof(header.identifier))) {
        WarnLog("Failed to read header");
        delete pack;
        return NULL;
    }

    if (strcmp(headerIdentifier, buffer) != 0) {
        WarnLog("Invalid pak header");
        delete pack;
        return NULL;
    }

    header.Version = readInt(in, 2);
    header.ContentVersion = readUTF(in);
    std::string name = readUTF(in);
    memset(header.PakName, 0, sizeof(header.PakName));
    memcpy(header.PakName, name.c_str(), name.size());
    header.NbEntries = readInt(in, 4);

    for (uint32 i = 0; i < header.NbEntries; i++) {
        AssetFileEntry entry;
        ReadFileEntryFromPak(in, pack->header, &entry);

        pack->entries.push_back(entry);
    }
    return pack;
}

void AssetPack::ExtractPakFile(std::string file, int fileIndex)
{

    AssetPack pack{};
    AssetFileHeader header = pack.header;

    pack.stream = std::make_unique<std::ifstream>(file, std::ios::binary);

    std::ifstream& in = *pack.stream.get();

    if (!in.is_open()) {
        WarnLog("Failed to open " << file);
    }

    char buffer[128];
    const char* headerIdentifier = (const char*)header.identifier;

    if (!in.read(buffer, sizeof(header.identifier))) {
        WarnLog("Failed to read header");
        return;
    }

    if (strcmp(headerIdentifier, buffer) != 0) {
        WarnLog("Invalid pak header");
        return;
    }

    header.Version = readInt(in, 2);
    header.ContentVersion = readUTF(in);
    std::string name = readUTF(in);
    memset(header.PakName, 0, sizeof(header.PakName));
    memcpy(header.PakName, name.c_str(), name.size());
    header.NbEntries = readInt(in, 4);

    pack.header = header;

    printf("Extracting %s\n", file.c_str());

    for (uint32 i = 0; i < header.NbEntries; i++) {
        AssetFileEntry entry;
        ReadFileEntryFromPak(in, header, &entry);

        pack.entries.push_back(entry);
    }

    for (auto& entry : pack.entries) {
        std::filesystem::path path = entry.Path;
        path.remove_filename();

        if (strncmp(entry.Path, "FileEntry", 10) == 0) continue;

        try
        {
            if (!path.empty() && !std::filesystem::exists(path))
            {
                std::filesystem::create_directories(path);
            }
        }
        catch (const std::exception& e)
        {
            ErrorLog(e.what());
            //continue;
        }

        path = entry.Path;

        std::stringstream fil = pack.GetFile(entry);

        std::ofstream out(path, std::ios::binary);

        out << fil.rdbuf();
    }

    if (file.ends_with("000.zpk") || fileIndex != 0) {
     
        std::filesystem::path path = file;
        std::filesystem::path stem = path.stem();

        std::string oldPath = file;
        std::string newPath;
        newPath.resize(oldPath.size() - 7);
        memcpy(newPath.data(), oldPath.data(), newPath.size());

        std::string nextfile{ std::format("{}{:03d}.zpk", newPath, fileIndex + 1) };

        if (std::filesystem::exists(nextfile)) {
            ExtractPakFile(nextfile, fileIndex + 1);
        }

    }

}

std::stringstream AssetPack::GetFile(AssetFileEntry& entry) {

    semaphore.acquire();

    stream->clear();
    stream->seekg(entry.Offset);

    std::vector<char> Data(entry.Size);

    std::string entryStr = readUTF(*stream);

    if (entry.IsCompressed) {
        try {
            std::streampos cpos = stream->tellg();
            zlib::izlibstream in(*stream);
            in.read(Data.data(), entry.Size);
            in.close();
            std::streampos epos = stream->tellg();
            std::streampos size = epos - cpos;
        }
        catch (std::ios_base::failure e) {
            ErrorLog(e.what());
        }
        catch (zlib::zlib_error e) {
            ErrorLog(e.what());
        }
        catch (std::exception e) {
            ErrorLog(e.what());
        }
    }
    else {
        stream->read(Data.data(), entry.Size);
    }

    semaphore.release();

    if (header.Version >= ASSET_PACKER_VER_BLOCK_ENCRYPTION)
    {
        BlockXorDecrypt(Data.data(), Data.size());
    }
    else
    {
        for (int i = 0; i < Data.size(); i++)
        {
            Data[i] = Data[i] ^ 18;
        }
    }

    std::stringstream sstream;
    sstream.write(Data.data(), entry.Size);

    return sstream;
}
