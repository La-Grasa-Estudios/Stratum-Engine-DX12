#include "DX12DescriptorHeap.h"
#include "GlobalSharedData.h"

DX12::DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc)
{
	dxSharedData->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap));

	m_IncrementSize = dxSharedData->Device->GetDescriptorHandleIncrementSize(desc.Type);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < desc.NumDescriptors; i++)
	{
		m_DescriptorPtrs.push(handle.ptr);
		handle.Offset(m_IncrementSize);
	}

}

CD3DX12_CPU_DESCRIPTOR_HANDLE DX12::DynamicDescriptorHeap::PopDescriptorHandle()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = {};
	handle.ptr = m_DescriptorPtrs.top();
	m_DescriptorPtrs.pop();
	return handle;
}

void DX12::DynamicDescriptorHeap::PushDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle)
{
	m_DescriptorPtrs.push(handle.ptr);
}
