#pragma once

#include "RendererContext.h"

BEGIN_ENGINE

namespace Render
{
	class BackendInitializerD3D12 : public BackendInitializer
	{
		void InitializeBackend(Internal::Window* pWindow, RendererContext* pContext) override;
		void TerminateBackend(RendererContext* pContext) override;

		void Present(Internal::Window* pWindow, RendererContext* pContext) override;
		bool RequiresResize(Internal::Window* pWindow) override;

		void ImGuiInit(Internal::Window* window) override;
		void ImGuiBeginFrame() override;
		void ImGuiEndFrame(RendererContext* pContext) override;
		void ImGuiShutdown() override;

		GraphicsDeviceProperties GetGraphicsDeviceProperties() override;
		nvrhi::CommandListHandle mCommandList;
	};
}
END_ENGINE