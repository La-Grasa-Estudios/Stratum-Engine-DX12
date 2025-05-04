#include "CopyCommandBuffer.h"

using namespace ENGINE_NAMESPACE;

Render::CopyCommandBuffer::CopyCommandBuffer()
{
	nvrhi::CommandListParameters params{};
	params.queueType = nvrhi::CommandQueue::Graphics;

	if (RendererContext::get_api() == RendererAPI::DX11)
	{
		params.queueType = nvrhi::CommandQueue::Graphics;
	}

	mCommandList = RendererContext::GetDevice()->createCommandList(params);
	mEventQuery = RendererContext::GetDevice()->createEventQuery();
}

void Render::CopyCommandBuffer::Begin()
{
	mCommandList->open();
}

void Render::CopyCommandBuffer::End()
{
	mCommandList->close();
}

void Render::CopyCommandBuffer::Submit()
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

void Render::CopyCommandBuffer::Wait()
{
	RendererContext::GetDevice()->waitEventQuery(mEventQuery);
}

void Render::CopyCommandBuffer::UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data)
{
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::CopyDest);
	mCommandList->writeBuffer(pBuffer->Handle, data, pBuffer->Size);
	mCommandList->setBufferState(pBuffer->Handle, nvrhi::ResourceStates::ConstantBuffer);
	mCommandList->commitBarriers();
}

nvrhi::ICommandList* Render::CopyCommandBuffer::GetNativeCommandList()
{
	mCommandList->commitBarriers();
	return mCommandList;
}
