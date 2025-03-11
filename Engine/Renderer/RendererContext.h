#pragma once

#include "znmsp.h"

BEGIN_ENGINE

namespace Internal {
	class Window;
}

namespace Render {

	enum class ClearBits {
		COLOR_BIT = 1,
		DEPTH_BIT = 2,
		STENCIL_BIT = 4,
	};

	enum class RendererAPI {

		DX11,
		DX12,

	};

	enum class RendererCapability {
		NONE,
		RENDERER_MULTI_THREAD_ACCESS,
		RENDERER_MULTITHREADED_CMD_LISTS,
	};

	struct GraphicsDeviceProperties
	{
		std::string Description;
		size_t DedicatedVideoMemory;
		size_t SharedVideoMemory;
		size_t UsedVideoMemory; // Rough estimation
	};

	class GraphicsPipeline;

	class RendererContext {

		inline static RendererAPI s_Api = RendererAPI::DX12;

	protected:

		bool m_VsyncState = false;

	public:

		RendererContext();

		void pre_glfw_window(ENGINE_NAMESPACE::Internal::Window* window);
		void initialize(ENGINE_NAMESPACE::Internal::Window* window);
		void terminate();

		void swap_buffers(ENGINE_NAMESPACE::Internal::Window* window);
		void set_viewport(uint32_t x, uint32_t y) {}
		void clear_front_buffer(uint32_t bits) {}

		void ImGuiInit(ENGINE_NAMESPACE::Internal::Window* window);
		void ImGuiBeginFrame();
		void ImGuiEndFrame();
		void ImGuiShutdown();

		void set_vsync(bool vsync);

		bool IsCapabilitySupported(RendererCapability capability);
		GraphicsDeviceProperties GetGraphicsDeviceProperties();

		void BlitTexture(void* src, void* dst, int slice = 0);

		static void set_api(RendererAPI api) {
			s_Api = api;
		}

		static RendererAPI get_api() {
			return s_Api;
		}

		DLLEXPORT static RendererContext* s_Context;

		void* InternalData;

	};

}

END_ENGINE
