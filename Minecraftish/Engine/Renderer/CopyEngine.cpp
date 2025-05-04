#include "CopyEngine.h"

#include "Core/Logger.h"

using namespace ENGINE_NAMESPACE;

void CopyEngine::Init()
{
	nvrhi::CommandListParameters params{};
	params.setQueueType(nvrhi::CommandQueue::Graphics);
	s_CommandList = Render::RendererContext::GetDevice()->createCommandList(params);
	s_EventQuery = Render::RendererContext::GetDevice()->createEventQuery();

	s_CommandQueue = nvrhi::CommandQueue::Graphics;

	if (Render::RendererContext::get_api() == Render::RendererAPI::DX11)
	{
		s_CommandQueue = nvrhi::CommandQueue::Graphics;
	}

	std::thread t(CopyThreadEntry);

	t.detach();
}

CopyingResource CopyEngine::WriteBuffer(nvrhi::BufferHandle handle, void* data, size_t size, bool* pResourceReady)
{

	auto commandList = GetAllocatorCommandList();

	commandList->open();

	CopyingResource resource = {};

	resource.pResourceReady = pResourceReady;
	resource.commandList = commandList;
	resource.eventQuery = Render::RendererContext::GetDevice()->createEventQuery();

	commandList->beginTrackingBufferState(handle, handle->getDesc().initialState);
	commandList->writeBuffer(handle, data, size);

	return resource;

}

CopyingResource CopyEngine::WriteTexture(nvrhi::ITexture* handle, bool* pResourceReady)
{
	auto commandList = GetAllocatorCommandList();

	CopyingResource resource = {};

	resource.pResourceReady = pResourceReady;
	resource.commandList = commandList;
	resource.eventQuery = Render::RendererContext::GetDevice()->createEventQuery();

	commandList->open();

	commandList->beginTrackingTextureState(handle, nvrhi::AllSubresources, handle->getDesc().initialState);

	return resource;
}

void CopyEngine::Submit(CopyingResource resource)
{
	resource.commandList->close();

	Render::RendererContext::GetDevice()->executeCommandList(resource.commandList, s_CommandQueue);
	Render::RendererContext::GetDevice()->setEventQuery(resource.eventQuery, s_CommandQueue);

	s_CopyLock.lock();

	s_CopyingTasks.push_back(resource);

	s_CopyLock.unlock();
}

void CopyEngine::CopyThreadEntry()
{
	while (1)
	{

		UpdateCopying();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void CopyEngine::UpdateCopying()
{

	if (!s_CommandList)
	{
		return;
	}

	s_CopyLock.lock();

	for (int i = 0; i < s_CopyingTasks.size(); i++)
	{

		CopyingResource& res = s_CopyingTasks[i];
		if (Render::RendererContext::GetDevice()->pollEventQuery(res.eventQuery))
		{
			Z_INFO_DEBUG("Resource: {} is ready", (void*)res.pResourceReady);

			*res.pResourceReady = true;
			
			std::scoped_lock lock(s_StagingLock);
			s_CopyCommandLists.push(res.commandList);

			s_CopyingTasks.erase(s_CopyingTasks.begin() + i);
			i--;
		}

	}

	s_CopyLock.unlock();

}

nvrhi::CommandListHandle CopyEngine::GetAllocatorCommandList()
{
	nvrhi::CommandListHandle commandList;

	std::scoped_lock lock(s_StagingLock);

	if (s_CopyCommandLists.empty())
	{
		nvrhi::CommandListParameters params{};
		params.setQueueType(s_CommandQueue);
		commandList = Render::RendererContext::GetDevice()->createCommandList(params);
	}
	else
	{
		commandList = s_CopyCommandLists.top();
		s_CopyCommandLists.pop();
	}

	return commandList;
}
