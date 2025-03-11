#include "IndexBuffer.h"

#include "DX12/GlobalSharedData.h"

using namespace ENGINE_NAMESPACE;

Render::IndexBuffer::IndexBuffer(Buffer* pBuffer)
{

	m_Buffer = pBuffer;
	NativeData = new DX12::DX12IndexBufferNativeData();

	D3D12_INDEX_BUFFER_VIEW view = {};

	auto dxBuffer = pBuffer->As<DX12::DX12BufferNativeData>();

	view.BufferLocation = dxBuffer->resource->GetGPUVirtualAddress();
	view.SizeInBytes = pBuffer->BufferDesc.Size;
	view.Format = DXGI_FORMAT_R32_UINT;

	this->As<DX12::DX12IndexBufferNativeData>()->BufferView = view;

}

Render::IndexBuffer::~IndexBuffer()
{
	delete this->As<DX12::DX12IndexBufferNativeData>();
}

Render::Buffer* Render::IndexBuffer::GetBuffer()
{
	return this->m_Buffer;
}
