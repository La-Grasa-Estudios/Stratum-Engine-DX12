#pragma once

#include "znmsp.h"
#include <nvrhi/nvrhi.h>

BEGIN_ENGINE

namespace Internal {
	class Window;
}

namespace Render {

	inline static constexpr uint32_t MaxInFlightFrames = 2;

	enum class ClearBits {
		COLOR_BIT = 1,
		DEPTH_BIT = 2,
		STENCIL_BIT = 4,
	};

	enum class RendererAPI {

		DX11,
		DX12,
		VULKAN,

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
	class BackendInitializerD3D12;
	class BackendInitializerD3D11;

	class DebugCallbackLogger : public nvrhi::IMessageCallback
	{
		void message(nvrhi::MessageSeverity severity, const char* messageText) override;
	};

	class RendererContext;

	/// <summary>
	/// A backend initializer does exactly what is sounds like.
	/// It creates a nvrhidevice from any api.
	/// Also sets one or both NvBackBuffers and NvFramebuffersRtv (In case of only one set frame index to 0)
	/// </summary>
	class BackendInitializer
	{
	public:
		virtual void InitializeBackend(Internal::Window* pWindow, RendererContext* pContext) = 0;
		virtual void TerminateBackend(RendererContext* pContext) = 0;

		virtual void Present(Internal::Window* pWindow, RendererContext* pContext) = 0;
		virtual bool RequiresResize(Internal::Window* pWindow) = 0;

		virtual void ImGuiInit(Internal::Window* window) = 0;
		virtual void ImGuiBeginFrame() = 0;
		virtual void ImGuiEndFrame(RendererContext* pContext) = 0;
		virtual void ImGuiShutdown() = 0;

		virtual GraphicsDeviceProperties GetGraphicsDeviceProperties() = 0;
	};

	class RendererContext {

		friend BackendInitializer;
		friend BackendInitializerD3D12;
		friend BackendInitializerD3D11;

		inline static RendererAPI s_Api = RendererAPI::DX12;
		DebugCallbackLogger mCallbackLogger{};

		BackendInitializer* pBackend;

		std::atomic_uint64_t mUsedVideoMemory = 0;

	protected:

		bool m_VsyncState = false;
		inline static nvrhi::DeviceHandle pDevice;
		std::vector<GraphicsPipeline*> swapChainPipelines;
		static void set_api(RendererAPI api) {
			s_Api = api;
		}

	public:

		RendererContext();
		void InitializeApi(RendererAPI api);

		void pre_glfw_window(ENGINE_NAMESPACE::Internal::Window* window);
		void initialize(ENGINE_NAMESPACE::Internal::Window* window);
		void Terminate();

		void present(ENGINE_NAMESPACE::Internal::Window* window);
		void set_viewport(uint32_t x, uint32_t y) {}
		void clear_front_buffer(uint32_t bits) {}

		void ImGuiInit(ENGINE_NAMESPACE::Internal::Window* window);
		void ImGuiBeginFrame();
		void ImGuiEndFrame();
		void ImGuiShutdown();

		static void VideoMemoryAdd(size_t size);
		static void VideoMemorySub(size_t size);
		static size_t GetSizeForFormat(uint32_t width, uint32_t height, uint32_t format);

		void set_vsync(bool vsync);

		bool IsCapabilitySupported(RendererCapability capability);
		GraphicsDeviceProperties GetGraphicsDeviceProperties();

		void BlitTexture(void* src, void* dst, int slice = 0);

		void RegisterBackBufferPipeline(GraphicsPipeline* pField);

		static RendererAPI get_api() {
			return s_Api;
		}

		static nvrhi::IDevice* GetDevice()
		{
			return pDevice.Get();
		}

		static RendererContext* s_Context;

		void* InternalData;

		nvrhi::TextureHandle NvBackBuffers[MaxInFlightFrames];
		nvrhi::TextureHandle NvDepthBuffers[MaxInFlightFrames];
		nvrhi::FramebufferHandle NvFramebufferRtvs[MaxInFlightFrames];
		uint32_t FrameIndex = 0;
		uint64_t FrameCount = 0;
		bool WasResized = false;

	};

}

END_ENGINE
