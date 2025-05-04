#include "Buffer.h"
#include "CopyEngine.h"

using namespace ENGINE_NAMESPACE;

#define nativeData reinterpret_cast<DX12::DX12BufferNativeData*>(NativePointer)

Render::Buffer::Buffer(const BufferDescription& desc)
{

	BufferDesc = desc;

	nvrhi::ResourceStates state = nvrhi::ResourceStates::Common;
	nvrhi::ResourceStates finalState = nvrhi::ResourceStates::Common;

	if (desc.Type == BufferType::VERTEX_BUFFER)
	{
		state = nvrhi::ResourceStates::VertexBuffer;
	}

	if (desc.Type == BufferType::INDEX_BUFFER)
	{
		state = nvrhi::ResourceStates::IndexBuffer;
	}

	if (desc.Type == BufferType::INDIRECT_BUFFER)
	{
		state = nvrhi::ResourceStates::UnorderedAccess;
	}

	auto bufferDesc = nvrhi::BufferDesc()
		.setByteSize(desc.Size)
		.setIsVertexBuffer(desc.Type == BufferType::VERTEX_BUFFER)
		.setIsIndexBuffer(desc.Type == BufferType::INDEX_BUFFER)
		.setInitialState(state)
		.setKeepInitialState(!desc.Immutable) // enable fully automatic state tracking
		.setDebugName("Buffer");

	bufferDesc.setDebugName("Buffer");

	bufferDesc.setIsDrawIndirectArgs(desc.Type == BufferType::INDIRECT_BUFFER);

	switch (desc.ComputeType)
	{
	case BufferComputeType::RAW:
		bufferDesc.setCanHaveRawViews(true);
		break;
	case BufferComputeType::TYPED:
		bufferDesc.setCanHaveTypedViews(true);
		break;
	case BufferComputeType::STRUCTURED:
		bufferDesc.setStructStride(desc.StructuredStride);
		break;
	default:
		break;
	}

	if (desc.Type == BufferType::VERTEX_BUFFER)
	{
		bufferDesc.setDebugName("Vertex Buffer");
	}

	if (desc.Type == BufferType::INDEX_BUFFER)
	{
		bufferDesc.setDebugName("Index Buffer");
	}

	if (desc.AllowComputeResourceUsage)
	{
		bufferDesc.setDebugName("Uav Buffer");
		bufferDesc.setCanHaveUAVs(true);
	}

	if (desc.Format != nvrhi::Format::UNKNOWN)
	{
		bufferDesc.setFormat(desc.Format);
		bufferDesc.setCanHaveTypedViews(true);
	}

	Handle = RendererContext::GetDevice()->createBuffer(bufferDesc);

	if (desc.pSysMem)
	{
		auto cmd = CopyEngine::WriteBuffer(Handle, desc.pSysMem, desc.Size, &this->m_ResourceReady);

		if (desc.Immutable)
		{
			cmd.commandList->setPermanentBufferState(Handle, state);
		}

		CopyEngine::Submit(cmd);
	}
	else
	{
		m_ResourceReady = true;
	}

	RendererContext::VideoMemoryAdd(BufferDesc.Size);

}

Render::Buffer::~Buffer()
{
	RendererContext::VideoMemorySub(BufferDesc.Size);
}

bool Render::Buffer::IsResourceReady()
{
	return m_ResourceReady;
}
