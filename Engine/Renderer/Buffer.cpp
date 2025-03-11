#include "Buffer.h"

#include "DX12/GlobalSharedData.h"
#include "DX12/DX12CopyEngine.h"

using namespace ENGINE_NAMESPACE;

#define nativeData reinterpret_cast<DX12::DX12BufferNativeData*>(NativePointer)

Render::Buffer::Buffer(const BufferDescription& desc)
{

	NativePointer = new DX12::DX12BufferNativeData();
	BufferDesc = desc;

	auto device = dxSharedData->Device.Get();

	ComPtr<ID3D12Resource> resource;

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (desc.AllowComputeResourceUsage)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	auto defaultDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uploadDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferResDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.Size, flags);

	device->CreateCommittedResource(
		&defaultDesc,
		D3D12_HEAP_FLAG_NONE, &bufferResDesc,
		D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&resource));

	

	if (desc.pSysMem)
	{
		ComPtr<ID3D12Resource> staging;

		device->CreateCommittedResource(
			&uploadDesc,
			D3D12_HEAP_FLAG_NONE, &bufferResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&staging));

		D3D12_SUBRESOURCE_DATA data = {};

		data.pData = desc.pSysMem;
		data.RowPitch = desc.Size;
		data.SlicePitch = desc.Size;

		DX12::CopyEngine::CopyResource(staging, resource, DX12::ResourceCopyType::BUFFER, &data, 1, &m_ResourceReady);
	}
	else
	{
		m_ResourceReady = true;
	}

	nativeData->resource = resource;

}

Render::Buffer::~Buffer()
{
	delete nativeData;
}

bool Render::Buffer::IsResourceReady()
{
	return m_ResourceReady;
}
