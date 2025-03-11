#include "UploadMemoryPool.h"
#include <corecrt_malloc.h>

constexpr size_t AllocatorHeapSize = 64 * 1024 * 1024;
constexpr size_t AllocatorMaxAllocs = 128;

void DX12::MemoryPoolAllocator::Init()
{
    MemoryBlock = (char*)malloc(AllocatorHeapSize);

    MemoryInfo[0] = { AllocatorHeapSize, 0 };
}

void* DX12::MemoryPoolAllocator::Allocate(size_t size)
{
    
    Semaphore.acquire();
    size_t aSize = size + sizeof(PoolAllocatorInfo);

    for (int i = 0; i < AllocatorMaxAllocs; i++) {
        PoolAllocatorInfo alloc = MemoryInfo[i];
        if (alloc.length >= aSize) {
            MemoryInfo[i].length = 0;

            size_t diff = alloc.length - aSize;
            alloc.length -= diff;
            
            PoolAllocatorInfo newAlloc = { diff, alloc.offset + aSize };
            Push(newAlloc);

            PoolAllocatorInfo* pAlloc = (PoolAllocatorInfo*)(MemoryBlock + alloc.offset);
            pAlloc->length = alloc.length;
            pAlloc->offset = alloc.offset;

            Semaphore.release();

            return (MemoryBlock + alloc.offset + sizeof(PoolAllocatorInfo));
        }
    }

    Semaphore.release();

    return nullptr;
}

void DX12::MemoryPoolAllocator::Free(void* ptr)
{
    Semaphore.acquire();
    PoolAllocatorInfo used = *(PoolAllocatorInfo*)((char*)ptr - sizeof(PoolAllocatorInfo));

    for (int i = 0; i < AllocatorMaxAllocs; i++) {
        PoolAllocatorInfo block = MemoryInfo[i];

        if (!block.length) continue;

        if (block.offset == used.offset + used.length) {
            used.length += block.length;
            MemoryInfo[i].length = 0;
        }
    }

    Push(used);
    Semaphore.release();
}

size_t DX12::MemoryPoolAllocator::GetTexture2DSize(uint32_t width, uint32_t height, int format)
{
    return g_ImageByteSizeTable[format] * width * height;
}

void DX12::MemoryPoolAllocator::Push(PoolAllocatorInfo info)
{
    for (int i = 0; i < AllocatorMaxAllocs; i++)
    {
        PoolAllocatorInfo block = MemoryInfo[i];

        if (!block.length)
        {
            MemoryInfo[i] = info;
            break;
        }
    }
}
