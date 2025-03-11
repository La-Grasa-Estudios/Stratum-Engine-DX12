#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <semaphore>

#include "znmsp.h"

BEGIN_ENGINE

typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#define ASSET_PACKER_VER 103
#define ASSET_PACKER_VER_B64 101
#define ASSET_PACKER_VER_B64_XOR 103
#define ASSET_PACKER_VER_BLOCK_ENCRYPTION 102

struct AssetFileHeader {
    uint8 identifier[5] = {"ZPAK"};
    uint16 Version = ASSET_PACKER_VER;
    std::string ContentVersion;
    uint8 PakName[50];
    uint32 NbEntries = 0;
};

struct AssetFileEntry {
    char* Path;
    uint16 PathSize;
    bool IsCompressed = false;
    uint32 Size = 0;
    uint32 CompressedSize = 0;
    uint64 Offset = 0;
};

class AssetPack
{
public:

    AssetFileHeader header;
    std::vector<AssetFileEntry> entries;
    std::unique_ptr<std::ifstream> stream;

    std::binary_semaphore semaphore = std::binary_semaphore(1);

    DLLEXPORT static AssetPack* MountFile(std::string pack);
    DLLEXPORT static void ExtractPakFile(std::string pack, int fileIndex = 0);

    DLLEXPORT std::stringstream GetFile(AssetFileEntry& entry);

    static void BlockXorEncrypt(char* ptr, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            size_t index = i % xorBlockSize;
            ptr[i] = ptr[i] ^ blockKeys[index];
        }
    }

    // Its the same shit as BlockXorEncrypt, this is just for naming
    static void BlockXorDecrypt(char* ptr, size_t size)
    {
        BlockXorEncrypt(ptr, size);
    }

    inline static constexpr int xorBlockSize = 16;
    inline static const char blockKeys[16] =
    {
        0x57, 0x6D, 0x6C, 0x79, 0x59, 0x32, 0x39, 0x75, 0x52, 0x57, 0x35, 0x6E, 0x61, 0x57, 0x35, 0x6C
    };

private:
};


struct AssetPacker {
    
    bool Compress = false;

    DLLEXPORT void Pack(std::string directory, std::string out, bool dirIsRoot, const char** pIgnore = NULL, int32_t nbIgnore = 0);

};

END_ENGINE

