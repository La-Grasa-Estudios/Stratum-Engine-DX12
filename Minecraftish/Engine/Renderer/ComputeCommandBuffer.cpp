#include "ComputeCommandBuffer.h"
#include "GraphicsCommandBuffer.h"

#include "Core/Logger.h"
#include "Core/Timer.h"

#include <thread>
#include <nvrhi/utils.h>

using namespace ENGINE_NAMESPACE;

Render::ComputeCommandBuffer::ComputeCommandBuffer(bool async)
{
	nvrhi::CommandListParameters params{};
	params.queueType = nvrhi::CommandQueue::Graphics;

	if (RendererContext::get_api() == RendererAPI::DX11 || !async)
	{
		params.queueType = nvrhi::CommandQueue::Graphics;
	}

	mCommandList = RendererContext::GetDevice()->createCommandList(params);
	mEventQuery = RendererContext::GetDevice()->createEventQuery();
	mConstantBuffers = {};
	mTextureUnits = {};
	mSamplerUnits = {};
	mTextureCompute = {};
	mBufferCompute = {};
	mSetComputePipeline = NULL;
	mCurrentPipeline = NULL;
}

Render::ComputeCommandBuffer::ComputeCommandBuffer(GraphicsCommandBuffer* pCmdBuffer)
{
	mConstantBuffers = {};
	mTextureUnits = {};
	mSamplerUnits = {};
	mTextureCompute = {};
	mBufferCompute = {};
	mBufferUnits = {};
	mSetComputePipeline = NULL;
	mCurrentPipeline = NULL;
	mIsOwner = false;
	mCommandList = pCmdBuffer->GetNativeCommandList();
}

void Render::ComputeCommandBuffer::Begin()
{
	mAutomaticBarriers = true;
	ClearState();
	mCommandList->setEnableAutomaticBarriers(mAutomaticBarriers);
	if (mIsOwner)
		mCommandList->open();
}

void Render::ComputeCommandBuffer::End()
{
	if (mIsOwner)
		mCommandList->close();
}

void Render::ComputeCommandBuffer::Submit()
{
	if (mIsOwner)
	{
		RendererContext::GetDevice()->executeCommandList(mCommandList);

		if (RendererContext::get_api() == RendererAPI::DX11)
		{
			RendererContext::GetDevice()->setEventQuery(mEventQuery, nvrhi::CommandQueue::Graphics);
		}
		else
		{
			RendererContext::GetDevice()->setEventQuery(mEventQuery, nvrhi::CommandQueue::Graphics);
		}
	}
}

void Render::ComputeCommandBuffer::Wait()
{
	if (!mIsOwner)
		return;
	RendererContext::GetDevice()->waitEventQuery(mEventQuery);
}

void Render::ComputeCommandBuffer::ClearState()
{
	mConstantBuffers = {};
	mTextureUnits = {};
	mSamplerUnits = {};
	mTextureCompute = {};
	mBufferCompute = {};
	mBufferUnits = {};
	mSetComputePipeline = NULL;
	mCurrentPipeline = NULL;
}

void Render::ComputeCommandBuffer::SetComputePipeline(ComputePipeline* pipeline)
{
	mCurrentPipeline = pipeline;
	mSetComputePipeline = pipeline->Handle;
	mComputeStateDirty = true;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot)
{
#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!buffer->IsResourceReady())
	{
		std::this_thread::yield();
#ifdef _DEBUG
		DidWait = true;
#endif
	}

#ifdef _DEBUG
	if (DidWait)
	{
		Z_WARN("CPU Thread stalled for {}ms waiting for resource to be in a ready state! Resource Pointer: {}", timer.GetMillis(), (void*)buffer);
	}
#endif
	mConstantBuffers[slot] = buffer->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetTextureResource(ImageResource* texture, uint32_t slot)
{
#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!texture->IsResourceReady())
	{
		std::this_thread::yield();
#ifdef _DEBUG
		DidWait = true;
#endif
	}

#ifdef _DEBUG
	if (DidWait)
	{
		Z_WARN("CPU Thread stalled for {}ms waiting for resource to be in a ready state! Resource Pointer: {}", timer.GetMillis(), (void*)texture);
	}
#endif

	mTextureUnits[slot] = texture->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetTextureResource(Texture3D* texture, uint32_t slot)
{
	mTextureUnits[slot] = texture->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetTextureSampler(TextureSampler* sampler, uint32_t slot)
{
	mSamplerUnits[slot] = sampler->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetBufferResource(Buffer* buffer, uint32_t slot)
{
	mBufferUnits[slot] = buffer->Handle;
	mTextureUnits[slot] = nullptr;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetBufferCompute(Buffer* buffer, uint32_t slot)
{
	mBufferCompute[slot] = buffer->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetTextureCompute(ImageResource* texture, uint32_t slot)
{
	mTextureCompute[slot] = texture->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::SetTextureCompute(Texture3D* texture, uint32_t slot)
{
	mTextureCompute[slot] = texture->Handle;
	mBindingStateDirty = true;
}

void Render::ComputeCommandBuffer::PushConstants(void* ptr, size_t size)
{
	UpdateComputeState();
	mCommandList->setPushConstants(ptr, size);
}

void Render::ComputeCommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	UpdateComputeState();
	mCommandList->dispatch(x, y, z);
}

void Render::ComputeCommandBuffer::UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data)
{
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::CopyDest);
	mCommandList->writeBuffer(pBuffer->Handle, data, pBuffer->Size);
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::ConstantBuffer);
	mCommitedAnyConstantBuffer = true;
}

void Render::ComputeCommandBuffer::Barrier(ImageResource* resource)
{
	nvrhi::utils::TextureUavBarrier(mCommandList, resource->Handle);
}

void Render::ComputeCommandBuffer::Barrier(Texture3D* resource)
{
	nvrhi::utils::TextureUavBarrier(mCommandList, resource->Handle);
}

void Render::ComputeCommandBuffer::Barrier(Buffer* resource)
{
	nvrhi::utils::BufferUavBarrier(mCommandList, resource->Handle);
}

void Render::ComputeCommandBuffer::CommitBarriers()
{
	mCommandList->commitBarriers();
}

void Render::ComputeCommandBuffer::ToggleAutomaticBarrierPlacement()
{
	mAutomaticBarriers = !mAutomaticBarriers;
	mCommandList->setEnableAutomaticBarriers(mAutomaticBarriers);
	mCommandList->commitBarriers();
}

nvrhi::ICommandList* Render::ComputeCommandBuffer::GetNativeCommandList()
{
	return mCommandList;
}

void Render::ComputeCommandBuffer::UpdateComputeState()
{
	if (mCommitedAnyConstantBuffer)
	{
		mCommandList->commitBarriers();
		mCommitedAnyConstantBuffer = false;
	}

	if (!mComputeStateDirty && !mBindingStateDirty)
	{
		return;
	}

	bool updateBindingState = false;

	if (mBindingStateDirty)
	{
		updateBindingState = true;
		auto bindingSetDesc = nvrhi::BindingSetDesc();

		auto& bindingItems = mCurrentPipeline->ShaderDesc.BindingItems;

		nvrhi::static_vector<nvrhi::BindingSetItem, 32> bindingSetItems;

		for (auto& item : mCurrentPipeline->ShaderDesc.BindingItems)
		{
			if (item.type == nvrhi::ResourceType::PushConstants)
			{
				bindingSetItems.push_back(nvrhi::BindingSetItem::PushConstants(item.slot, item.size));
			}
		}

		for (int i = 0; i < mTextureUnits.size(); i++)
		{
			if (mTextureUnits[i] != 0)
			{
				bindingSetItems.push_back(nvrhi::BindingSetItem::Texture_SRV(i, mTextureUnits[i]));
			}
		}

		for (int i = 0; i < mBufferUnits.size(); i++)
		{
			if (mBufferUnits[i] != 0)
			{
				if (mBufferUnits[i]->getDesc().canHaveRawViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(i, mBufferUnits[i]));
				else if (mBufferUnits[i]->getDesc().canHaveTypedViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::TypedBuffer_SRV(i, mBufferUnits[i]));
				else if (mBufferUnits[i]->getDesc().structStride != 0)
					bindingSetItems.push_back(nvrhi::BindingSetItem::StructuredBuffer_SRV(i, mBufferUnits[i]));
			}
		}

		for (int i = 0; i < mSamplerUnits.size(); i++)
		{
			if (mSamplerUnits[i] != 0)
			{
				bindingSetItems.push_back(nvrhi::BindingSetItem::Sampler(i, mSamplerUnits[i]));
			}
		}

		for (int i = 0; i < mConstantBuffers.size(); i++)
		{
			if (mConstantBuffers[i] != 0)
			{
				bindingSetItems.push_back(nvrhi::BindingSetItem::ConstantBuffer(i, mConstantBuffers[i]));
			}
		}

		for (int i = 0; i < mBufferCompute.size(); i++)
		{
			if (mBufferCompute[i] != 0)
			{
				if (mBufferCompute[i]->getDesc().canHaveRawViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_UAV(i, mBufferCompute[i]));
				else if (mBufferCompute[i]->getDesc().canHaveTypedViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::TypedBuffer_UAV(i, mBufferCompute[i]));
				else if (mBufferCompute[i]->getDesc().structStride != 0)
					bindingSetItems.push_back(nvrhi::BindingSetItem::StructuredBuffer_UAV(i, mBufferCompute[i]));
			}
		}

		for (int i = 0; i < mTextureCompute.size(); i++)
		{
			if (mTextureCompute[i] != 0)
			{
				bindingSetItems.push_back(nvrhi::BindingSetItem::Texture_UAV(i, mTextureCompute[i]));
			}
		}

		for (int i = 0; i < bindingItems.size(); i++)
		{
			auto& item = bindingItems[i];
			nvrhi::BindingSetItem* match = NULL;
			uint32_t matchIndex = 0;
			for (int j = 0; j < bindingSetItems.size(); j++)
			{
				if (bindingSetItems[j].type == item.type && bindingSetItems[j].slot == item.slot)
				{
					match = &bindingSetItems[j];
					matchIndex = j;
					break;
				}
			}
			if (match)
			{
				bindingSetDesc.addItem(*match);
			}
		}

		if (mBindingSet)
		{
			mBindingSet.Reset();
		}

		mBindingSet = RendererContext::GetDevice()->createBindingSet(bindingSetDesc, mCurrentPipeline->BindingLayout);
		mBindingStateDirty = false;
	}

	auto computeState = nvrhi::ComputeState();
	computeState.setPipeline(mSetComputePipeline);
	computeState.addBindingSet(mBindingSet);

	mCommandList->setComputeState(computeState);

	mComputeStateDirty = false;
}
