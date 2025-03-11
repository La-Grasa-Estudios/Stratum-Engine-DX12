#pragma once

#include "znmsp.h"
#include "Core/Ref.h"

BEGIN_ENGINE

namespace Render {

	enum class BufferUsage {
		DEFAULT,
		DYNAMIC,
		STAGING,
	};

	enum class BufferType {
		STORAGE,
		VERTEX_BUFFER,
		INDEX_BUFFER,
	};

	struct BufferDescription
	{
		bool AllowComputeResourceUsage = false;
		bool AllowWritesFromMainMemory = false;
		bool AllowReadsToMainMemory = false;

		BufferUsage Usage = BufferUsage::DEFAULT;
		BufferType Type = BufferType::STORAGE;

		void* pSysMem = NULL; // Default data for the buffer
		size_t Size;

		size_t StructuredStride = 4; // Used in compute resources
	};

	class Buffer {

	public:

		DLLEXPORT Buffer(const BufferDescription& desc);
		DLLEXPORT ~Buffer();

		template<typename T>
		T* As()
		{
			return (T*)NativePointer;
		}

		void* Map();
		void Unmap();
		bool IsResourceReady();

		BufferDescription BufferDesc;

		void* NativePointer;

	private:

		bool m_ResourceReady = false;

	};

}

END_ENGINE