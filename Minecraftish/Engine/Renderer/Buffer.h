#pragma once

#include "znmsp.h"
#include "RendererContext.h"

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
		INDIRECT_BUFFER,
	};

	enum class BufferComputeType
	{
		UNKNOWN,
		RAW,
		TYPED,
		STRUCTURED,
	};

	/// <summary>
	/// Creates a buffer in GPU memory.
	/// Uploads are managed through CopyEngine.cpp so no need to worry about staging
	/// </summary>
	struct BufferDescription
	{
		bool AllowComputeResourceUsage = false; // Needed to be used as a UAV in a compute or graphics command buffer
		bool AllowWritesFromMainMemory = false; // Deprecated ¯\_(ツ)_/¯
		bool AllowReadsToMainMemory = false; // Also deprecated ¯\_(ツ)_/¯
		bool Immutable = false; // Used by vertex and index buffers, marks the buffers as immutable so no state tracking is needed but you cannot update it even from a compute shader

		BufferUsage Usage = BufferUsage::DEFAULT; // Deprecated ¯\_(ツ)_/¯
		BufferType Type = BufferType::STORAGE; // Defines the buffer type if you want to bind it as index or vertex buffer it needs to be marked as the corresponding type, same for indirect buffer.
		BufferComputeType ComputeType = BufferComputeType::UNKNOWN; // Matches the HLSL type with the C++ type ex: HLSL (StructuredBuffer) C++ (ComputeType = BufferComputeType::STRUCTURED)

		nvrhi::Format Format = nvrhi::Format::UNKNOWN; // Used by typed buffers needs to match the HLSL typed buffer (Buffer<float4> needs to be RGBAXX format)

		void* pSysMem = NULL; // Default data for the buffer it will be initialized with this, the only way to initialize an immutable buffer is with the default data pointer, memory is copied by the CopyEngine so passed pointer may be safely freed after buffer creation
		size_t Size; // Size of the buffer, also used as the size of the pSysMem pointer

		size_t StructuredStride = 4; // Used by the STRUCTURED buffer type needs to match HLSL struct size
	};

	class Buffer {

	public:

		Buffer() = default;
		Buffer(const BufferDescription& desc);
		~Buffer();

		void* Map(); // TO DO: Actually implement this function
		void Unmap(); // Same as above :D
		bool IsResourceReady(); // Flag set by CopyEngine to ensure the buffer is ready by the time it is referenced by the gpu

		BufferDescription BufferDesc; // Description used at creation time

		nvrhi::BufferHandle Handle;

	private:

		bool m_ResourceReady = false;

	};

}

END_ENGINE