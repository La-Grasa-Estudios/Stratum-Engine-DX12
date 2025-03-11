#pragma once

#include <vector>
#include <semaphore>

namespace DX12 {

	struct PoolAllocatorInfo
	{
		size_t length;
		size_t offset;
	};

	class MemoryPoolAllocator {

	public:

		static void Init();

		static void* Allocate(size_t size);
		static void Free(void* ptr);

		static size_t GetTexture2DSize(uint32_t width, uint32_t height, int format);

	private:

		inline static const float g_ImageByteSizeTable[] =
		{
			1, 2, 4,
			1, 2, 4,
			2, 4, 8,
			2, 4, 8,
			2, 4, 8,
			4, 8, 12, 16,
			2,
			4,
			4,
			4,
			2,
			4,
			0.25f,
			4,
		};

		static void Push(PoolAllocatorInfo info);

		inline static char* MemoryBlock;

		inline static PoolAllocatorInfo MemoryInfo[128];
		inline static std::binary_semaphore Semaphore = std::binary_semaphore{ 1 };

	};

}