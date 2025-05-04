#pragma once

#include "RendererContext.h"

BEGIN_ENGINE

namespace Render
{
	class BackendInitializerD3D11 : public BackendInitializer
	{
	public:
		void InitializeBackend(Internal::Window* pWindow, RendererContext* pContext) override;
		void Present(Internal::Window* pWindow, RendererContext* pContext) override;
		bool RequiresResize(Internal::Window* pWindow) override;
	};
}

END_ENGINE
