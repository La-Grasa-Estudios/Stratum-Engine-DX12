#include "DX12CopyEngine.h"
#include "UploadMemoryPool.h"

void DX12::CopyEngine::Init()
{
	auto device = dxSharedData->Device.Get();

	for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
	{
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&s_CommandAllocators[i]));
	}

	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, s_CommandAllocators[0].Get(), NULL, IID_PPV_ARGS(&s_CommandList));
	s_CommandList->Close();

	s_Fence = std::make_shared<DX12::DX12Fence>();

	std::thread t(CopyThreadEntry);

	t.detach();
}

void DX12::CopyEngine::CopyResource(ComPtr<ID3D12Resource> src, ComPtr<ID3D12Resource> dst, ResourceCopyType type,
								D3D12_SUBRESOURCE_DATA* pSubResourceData, uint32_t numResourceData, bool* pResourceReady)
{

	CopyingResource resource = {};

	resource.src = src;
	resource.dst = dst;
	resource.FenceValue = UINT64_MAX;
	resource.pResourceReady = pResourceReady;
	resource.SubResourceData.reserve(numResourceData);
	
	for (int i = 0; i < numResourceData; i++)
	{
		D3D12_SUBRESOURCE_DATA data = pSubResourceData[i];
		
		if (type == ResourceCopyType::BUFFER)
		{
			void* newBlock = MemoryPoolAllocator::Allocate(data.RowPitch);
			memcpy(newBlock, data.pData, data.RowPitch);
			data.pData = newBlock;
		}

		resource.SubResourceData.push_back(data);
	}

	s_CopyLock.lock();

	s_CopyingTasks.push_back(resource);

	s_CopyLock.unlock();

}

void DX12::CopyEngine::CopyThreadEntry()
{
	while (1)
	{

		UpdateCopying();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void DX12::CopyEngine::UpdateCopying()
{
	auto allocator = s_CommandAllocators[dxSharedData->FrameIndex].Get();
	allocator->Reset();
	s_CommandList->Reset(allocator, NULL);

	s_CopyLock.lock();

	for (int i = 0; i < s_CopyingTasks.size(); i++)
	{

		CopyingResource& res = s_CopyingTasks[i];

		// Kick off new copy DMA
		if (res.FenceValue == UINT64_MAX)
		{
			UpdateSubresources(s_CommandList.Get(),
				res.dst.Get(), res.src.Get(),
				0, 0, res.SubResourceData.size(), res.SubResourceData.data());
			*res.pResourceReady = false;

			for (int i = 0; i < res.SubResourceData.size(); i++)
			{
				MemoryPoolAllocator::Free((void*)res.SubResourceData[0].pData);
			}

		}

	}

	s_CommandList->Close();

	ID3D12CommandList* lists[] =
	{
		s_CommandList.Get(),
	};

	dxSharedData->CopyQueue->ExecuteCommandLists(1, lists);

	uint64_t signalValue = s_Fence->Signal(dxSharedData->CopyQueue.Get(), s_FrameIndex);

	s_CopyLock.unlock();

	for (int i = 0; i < s_CopyingTasks.size(); i++)
	{

		CopyingResource& res = s_CopyingTasks[i];

		if (res.FenceValue == UINT64_MAX)
		{
			res.FenceValue = signalValue;
		} else if (s_Fence->HasFenceReachedValue(res.FenceValue))
		{
			*res.pResourceReady = true;

			s_CopyingTasks.erase(s_CopyingTasks.begin() + i);
			i--;
		}

	}

	s_FrameIndex = (s_FrameIndex + 1) % DX12::s_MaxInFlightFrames;

	s_Fence->WaitForSingleObject(s_FrameIndex);

}
