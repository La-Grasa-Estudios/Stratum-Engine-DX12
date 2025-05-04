#pragma once

#include <cstdint>
#include <wrl/client.h>

#include "directx/d3d12.h"
#include "DX12Constants.h"

namespace DX12
{
	class DX12Fence
	{
	public:

		DX12Fence();
		~DX12Fence();

		uint64_t Signal(ID3D12CommandQueue* pQueue, uint64_t frameIndex = UINT64_MAX);
		void WaitForSingleObject(uint64_t value = UINT64_MAX, uint64_t frameIndex = UINT64_MAX);
		bool HasFenceReachedValue(uint64_t value);
		void ResetValues();

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
		uint64_t m_FenceValue;
		uint64_t m_FrameFenceValues[DX12::s_MaxInFlightFrames] = {};
		void* m_FenceEvent;
	};
}