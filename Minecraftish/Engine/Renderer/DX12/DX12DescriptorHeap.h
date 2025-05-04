#pragma once

#include "directx/d3dx12.h"
#include <wrl/client.h>

#include <stack>

namespace DX12
{
	class DynamicDescriptorHeap
	{
	public:

		DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC desc);

		CD3DX12_CPU_DESCRIPTOR_HANDLE PopDescriptorHandle();
		void PushDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE handle);

	private:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
		uint32_t m_IncrementSize;

		std::stack<size_t> m_DescriptorPtrs;

	};
}