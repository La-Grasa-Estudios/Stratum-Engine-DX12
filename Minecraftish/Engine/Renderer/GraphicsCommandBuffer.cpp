#include "GraphicsCommandBuffer.h"

#include "Core/Logger.h"
#include "Core/Timer.h"

#include "Util/StackAllocator.h"
#include "BindlessDescriptorTable.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>
#include <thread>

using namespace ENGINE_NAMESPACE;

Render::GraphicsCommandBuffer::GraphicsCommandBuffer()
{
	nvrhi::CommandListParameters params{};
	params.queueType = nvrhi::CommandQueue::Graphics;
	mCommandList = RendererContext::GetDevice()->createCommandList(params);
}

Render::GraphicsCommandBuffer::~GraphicsCommandBuffer()
{
	
}

void Render::GraphicsCommandBuffer::Begin()
{
	uint64_t FrameIndex = RendererContext::s_Context->FrameCount;

	nvrhi::static_vector<BoundBindlessTable, 64> removePending = {};

	for (auto i = mBoundTables.begin(); i != mBoundTables.end(); ++i)
	{
		if (FrameIndex - i->FrameIndex > Render::MaxInFlightFrames + 1)
		{
			if (removePending.size() + 1 < 64)
				removePending.push_back(*i);
		}
	}

	for (auto& remove : removePending)
	{
		mBoundTables.erase(remove);
	}

	mConstantBuffers = {};
	mVertexBuffers = {};
	mTextureUnits = {};
	mSamplerUnits = {};
	mBufferResources = {};
	mBufferUavs = {};

	mIndirectBufferParams = nullptr;
	mSetDescriptorTable = nullptr;

	mCommandList->open();
}

void Render::GraphicsCommandBuffer::End()
{
	if (mCommitedAnyConstantBuffer)
	{
		mCommandList->commitBarriers();
	}
	mCommandList->close();
}

void Render::GraphicsCommandBuffer::Submit()
{
	RendererContext::GetDevice()->executeCommandList(mCommandList);
}

void Render::GraphicsCommandBuffer::SetPipeline(GraphicsPipeline* pipeline)
{

	if (!pipeline->PipelineHandle)
	{
		pipeline->UpdatePipeline();
	}

	mCurrentPipeline = pipeline;
	mSetGraphicsPipeline = pipeline->PipelineHandle;
	mGraphicsStateDirty = true;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot)
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

void Render::GraphicsCommandBuffer::SetVertexBuffer(VertexBuffer* buffer, uint32_t slot)
{
#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!buffer->GetBuffer()->IsResourceReady())
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

	mVertexBuffers[slot] = buffer->Handle;
	mGraphicsStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetIndexBuffer(IndexBuffer* buffer)
{

	mSetIndexBuffer = {};

	if (!buffer)
	{
		mGraphicsStateDirty = true;
		return;
	}

#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!buffer->GetBuffer()->IsResourceReady())
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

	mSetIndexBuffer.setBuffer(buffer->Handle);
	mSetIndexBuffer.setFormat(nvrhi::Format::R32_UINT);
	mGraphicsStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetFramebuffer(Framebuffer* framebuffer)
{
	if (!framebuffer)
	{
		mSetFramebuffer = NULL;
		return;
	}
	if (framebuffer->IsWindowFramebuffer())
	{
		mSetFramebuffer = RendererContext::s_Context->NvFramebufferRtvs[RendererContext::s_Context->FrameIndex].Get();
		return;
	}
	mSetFramebuffer = framebuffer->Handle;

	mCommandList->setResourceStatesForFramebuffer(mSetFramebuffer);
	mCommandList->commitBarriers();
}

void Render::GraphicsCommandBuffer::PushConstants(void* ptr, size_t size)
{
	UpdateGraphicsState();
	mCommandList->setPushConstants(ptr, size);
}

void Render::GraphicsCommandBuffer::SetTextureResource(ImageResource* texture, uint32_t slot)
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
	mBufferResources[slot] = nullptr;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetTextureResource(Texture3D* texture, uint32_t slot)
{
	mTextureUnits[slot] = texture->Handle;
	mBufferResources[slot] = nullptr;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetBufferResource(Buffer* buffer, uint32_t slot)
{
	mBufferResources[slot] = buffer->Handle;
	mTextureUnits[slot] = nullptr;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetTextureSampler(TextureSampler* sampler, uint32_t slot)
{
	mSamplerUnits[slot] = sampler->Handle;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetBufferUav(Buffer* buffer, uint32_t slot)
{
	mBufferUavs[slot] = buffer->Handle;
	mBindingStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetBindlessDescriptorTable(Ref<BindlessDescriptorTable> table)
{

	if (!table)
	{
		mSetDescriptorTable = nullptr;
		mGraphicsStateDirty = true;
		return;
	}

	mSetDescriptorTable = table->GetDescriptorTable();
	mGraphicsStateDirty = true;

	BoundBindlessTable bTable = { RendererContext::s_Context->FrameCount, table };

	mBoundTables.insert(bTable);
}

void Render::GraphicsCommandBuffer::SetIndirectBuffer(Buffer* buffer)
{
	if (!buffer)
	{
		mIndirectBufferParams = nullptr;
		mGraphicsStateDirty = true;
		return;
	}
	mIndirectBufferParams = buffer->Handle;
	mGraphicsStateDirty = true;
}

void Render::GraphicsCommandBuffer::SetViewport(Viewport* vp)
{
	mSetViewport = nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(vp->x, vp->width, vp->y, vp->height, 0.0f, 1.0f));
	mGraphicsStateDirty = true;
}

void Render::GraphicsCommandBuffer::Draw(uint32_t count)
{
	UpdateGraphicsState();
	auto args = nvrhi::DrawArguments()
		.setVertexCount(count);
	mCommandList->draw(args);
}

void Render::GraphicsCommandBuffer::DrawInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance)
{
	UpdateGraphicsState();
	auto args = nvrhi::DrawArguments()
		.setInstanceCount(instanceCount)
		.setStartInstanceLocation(baseInstance)
		.setVertexCount(count);
	mCommandList->draw(args);
}

void Render::GraphicsCommandBuffer::DrawIndirect(uint32_t count, uint64_t offsetBytes)
{
	UpdateGraphicsState();
	mCommandList->drawIndirect(offsetBytes, count);
}

void Render::GraphicsCommandBuffer::DrawIndexed(uint32_t count)
{
	UpdateGraphicsState();
	auto args = nvrhi::DrawArguments()
		.setVertexCount(count);
	mCommandList->drawIndexed(args);
}

void Render::GraphicsCommandBuffer::DrawIndexedInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance)
{
	UpdateGraphicsState();
	auto args = nvrhi::DrawArguments()
		.setInstanceCount(instanceCount)
		.setStartInstanceLocation(baseInstance)
		.setVertexCount(count);
	mCommandList->drawIndexed(args);
}

void Render::GraphicsCommandBuffer::DrawIndexedIndirect(uint32_t count)
{
	
}

void Render::GraphicsCommandBuffer::ClearBuffer(uint32_t index, const glm::vec4 color)
{
	if (!mSetFramebuffer)
	{
		return;
	}
	nvrhi::utils::ClearColorAttachment(mCommandList, mSetFramebuffer, index, nvrhi::Color(color.r, color.g, color.b, color.a));
}

void Render::GraphicsCommandBuffer::ClearDepth(float depth, uint32_t stencil)
{
	if (!mSetFramebuffer)
	{
		return;
	}
	nvrhi::utils::ClearDepthStencilAttachment(mCommandList, mSetFramebuffer, depth, stencil);
}

void Render::GraphicsCommandBuffer::UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data)
{
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::CopyDest);
	mCommandList->writeBuffer(pBuffer->Handle, data, pBuffer->Size);
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::ConstantBuffer);
	mCommitedAnyConstantBuffer = true;
}

void Render::GraphicsCommandBuffer::ClearVertexBuffers()
{
	mVertexBuffers = {};
}

nvrhi::ICommandList* Render::GraphicsCommandBuffer::GetNativeCommandList()
{
	return mCommandList;
}

void Render::GraphicsCommandBuffer::UpdateGraphicsState()
{

	if (mCommitedAnyConstantBuffer)
	{
		mCommandList->commitBarriers();
		mCommitedAnyConstantBuffer = false;
	}

	if (!mGraphicsStateDirty && !mBindingStateDirty)
	{
		return;
	}

	bool updateBindingState = false;

	if (mBindingStateDirty && !mCurrentPipeline->ShaderDesc.UseStaticBinding)
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

		for (int i = 0; i < mBufferResources.size(); i++)
		{
			if (mBufferResources[i] != 0)
			{
				if (mBufferResources[i]->getDesc().canHaveRawViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(i, mBufferResources[i]));
				else if (mBufferResources[i]->getDesc().canHaveTypedViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::TypedBuffer_SRV(i, mBufferResources[i]));
				else if (mBufferResources[i]->getDesc().structStride != 0)
					bindingSetItems.push_back(nvrhi::BindingSetItem::StructuredBuffer_SRV(i, mBufferResources[i]));
			}
		}

		for (int i = 0; i < mBufferUavs.size(); i++)
		{
			if (mBufferUavs[i] != 0)
			{
				if (mBufferUavs[i]->getDesc().canHaveRawViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_UAV(i, mBufferUavs[i]));
				else if (mBufferUavs[i]->getDesc().canHaveTypedViews)
					bindingSetItems.push_back(nvrhi::BindingSetItem::TypedBuffer_UAV(i, mBufferUavs[i]));
				else if (mBufferUavs[i]->getDesc().structStride != 0)
					bindingSetItems.push_back(nvrhi::BindingSetItem::StructuredBuffer_UAV(i, mBufferUavs[i]));
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

	auto graphicsState = nvrhi::GraphicsState();
	graphicsState.setPipeline(mSetGraphicsPipeline);
	graphicsState.setFramebuffer(mSetFramebuffer);
	graphicsState.setViewport(mSetViewport);

	if (mCurrentPipeline->ShaderDesc.UseStaticBinding)
	{

		if (!mCurrentPipeline->StaticBindingSet)
		{

			auto bindingSetDesc = nvrhi::BindingSetDesc();

			auto& bindingItems = mCurrentPipeline->ShaderDesc.BindingItems;

			for (int i = 0; i < bindingItems.size(); i++)
			{
				auto& item = bindingItems[i];
				nvrhi::BindingSetItem* match = NULL;
				uint32_t matchIndex = 0;
				for (int j = 0; j < mCurrentPipeline->ShaderDesc.StaticBindingItems.size(); j++)
				{
					if (mCurrentPipeline->ShaderDesc.StaticBindingItems[j].type == item.type && mCurrentPipeline->ShaderDesc.StaticBindingItems[j].slot == item.slot)
					{
						match = &mCurrentPipeline->ShaderDesc.StaticBindingItems[j];
						matchIndex = j;
						break;
					}
				}
				if (match)
				{
					bindingSetDesc.addItem(*match);
				}
			}

			mCurrentPipeline->StaticBindingSet = RendererContext::GetDevice()->createBindingSet(bindingSetDesc, mCurrentPipeline->BindingLayout);
		}

		graphicsState.addBindingSet(mCurrentPipeline->StaticBindingSet);

	}
	else
	{
		graphicsState.addBindingSet(mBindingSet);
	}

	if (mSetDescriptorTable)
	{
		graphicsState.addBindingSet(mSetDescriptorTable);
	}

	for (int i = 0; i < mVertexBuffers.size(); i++)
	{
		if (mVertexBuffers[i])
		{
			nvrhi::VertexBufferBinding binding{};

			binding.setBuffer(mVertexBuffers[i]);
			binding.setSlot(i);

			graphicsState.addVertexBuffer(binding);
		}
	}

	if (mSetIndexBuffer.buffer)
	{
		graphicsState.setIndexBuffer(mSetIndexBuffer);
	}

	if (mIndirectBufferParams)
	{
		graphicsState.setIndirectParams(mIndirectBufferParams);
	}

	mCommandList->setGraphicsState(graphicsState);

	mGraphicsStateDirty = false;
}

std::size_t Render::GraphicsCommandBuffer::BindlessTableHasher::operator()(const BoundBindlessTable& item) const
{
	size_t hash = 0;
	nvrhi::hash_combine(hash, item.Table->GetDescriptorTable());
	return hash;
}

bool Render::GraphicsCommandBuffer::BindlessTableEqual::operator()(const BoundBindlessTable& a, const BoundBindlessTable& b) const
{
	return (uintptr_t)a.Table.get() == (uintptr_t)b.Table.get() && a.FrameIndex == b.FrameIndex;
}
