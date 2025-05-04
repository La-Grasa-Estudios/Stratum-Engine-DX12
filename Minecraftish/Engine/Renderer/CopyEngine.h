#pragma once

#include "RendererContext.h"
#include <mutex>
#include <thread>
#include <stack>

BEGIN_ENGINE

	enum class ResourceCopyType
	{
		BUFFER,
		TEXTURE,
	};

	struct CopyingResource
	{
		bool* pResourceReady;
		nvrhi::EventQueryHandle eventQuery;
		nvrhi::CommandListHandle commandList;
	};

	class CopyEngine
	{

	public:

		static void Init();
		static CopyingResource WriteBuffer(nvrhi::BufferHandle handle, void* data, size_t size, bool* pResourceReady);
		static CopyingResource WriteTexture(nvrhi::ITexture* handle, bool* pResourceReady);
		static void Submit(CopyingResource resource);

	private:

		static void CopyThreadEntry();

		static void UpdateCopying();

		static nvrhi::CommandListHandle GetAllocatorCommandList();

		inline static nvrhi::CommandListHandle s_CommandList;
		inline static nvrhi::EventQueryHandle s_EventQuery;

		inline static std::mutex s_CopyLock;
		inline static std::mutex s_StagingLock;
		inline static uint64_t s_FrameIndex = 0;

		inline static std::vector<CopyingResource> s_CopyingTasks;

		inline static std::stack<nvrhi::CommandListHandle> s_CopyCommandLists;
		inline static nvrhi::CommandQueue s_CommandQueue;

	};
END_ENGINE