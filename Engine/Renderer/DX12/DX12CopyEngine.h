#pragma once

#include "GlobalSharedData.h"
#include <mutex>
#include <thread>

namespace DX12
{
	enum class ResourceCopyType
	{
		BUFFER,
		TEXTURE,
	};

	class CopyEngine
	{

	public:

		static void Init();
		static void CopyResource(ComPtr<ID3D12Resource> src, ComPtr<ID3D12Resource> dst, ResourceCopyType type, D3D12_SUBRESOURCE_DATA* pSubResourceData, uint32_t numResourceData, bool* pResourceReady);

	private:

		static void CopyThreadEntry();

		static void UpdateCopying();

		struct CopyingResource
		{
			ComPtr<ID3D12Resource> src;
			ComPtr<ID3D12Resource> dst;
			std::vector<D3D12_SUBRESOURCE_DATA> SubResourceData;
			bool* pResourceReady;
			uint64_t FenceValue;
		};

		inline static ComPtr<ID3D12GraphicsCommandList> s_CommandList;
		inline static ComPtr<ID3D12CommandAllocator> s_CommandAllocators[DX12::s_MaxInFlightFrames];

		inline static std::shared_ptr<DX12Fence> s_Fence;
		inline static std::mutex s_CopyLock;
		inline static uint64_t s_FrameIndex = 0;

		inline static std::vector<CopyingResource> s_CopyingTasks;

	};
}