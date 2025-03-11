#include "VertexBuffer.h"

#include "DX12/GlobalSharedData.h"

using namespace ENGINE_NAMESPACE;

Render::VertexBuffer::VertexBuffer(Buffer* pBuffer)
{

	m_Buffer = pBuffer;
	NativeData = new DX12::DX12VertexBufferNativeData();

	D3D12_VERTEX_BUFFER_VIEW view = {};

	auto dxBuffer = pBuffer->As<DX12::DX12BufferNativeData>();

	view.BufferLocation = dxBuffer->resource->GetGPUVirtualAddress();
	view.SizeInBytes = pBuffer->BufferDesc.Size;
	view.StrideInBytes = pBuffer->BufferDesc.StructuredStride;

	this->As<DX12::DX12VertexBufferNativeData>()->BufferView = view;

}

Render::VertexBuffer::~VertexBuffer()
{
	delete this->As<DX12::DX12VertexBufferNativeData>();
}

Render::Buffer* Render::VertexBuffer::GetBuffer()
{
	return this->m_Buffer;
}
