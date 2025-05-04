#include "RendererContext.h"

using namespace ENGINE_NAMESPACE::Render;

RendererContext* RendererContext::s_Context = NULL;

#include "Core/Window.h"
#include "Core/Logger.h"

#include "CopyEngine.h"

#include "GraphicsPipeline.h"
#include "NVRHI_d3d12.h"

#include "Thirdparty/imgui/imgui_impl_sdl3.h"

inline static const float g_ImageBPPTable[] =
{
    1, 2, 4,
    1, 2, 4,
    2, 4, 8,
    2, 4, 8,
    2, 4, 8,
    4, 8, 12, 16,
    2,
    4,
    4,
    4,
    2,
    4,
    0.25f,
    4,
};

size_t AlignTo(size_t size, size_t alignment)
{
    if (size % alignment != 0)
    {
        return size + (size % alignment);
    }
    return size;
}

void RendererContext::InitializeApi(RendererAPI api)
{
    if (api == RendererAPI::DX12)
    {
        pBackend = new BackendInitializerD3D12();
    }
    if (api == RendererAPI::DX11)
    {
        //pBackend = new BackendInitializerD3D11();
    }
    set_api(api);
}

RendererContext::RendererContext()
{
    s_Context = this;
}

void RendererContext::pre_glfw_window(ENGINE_NAMESPACE::Internal::Window* window)
{
}

void RendererContext::initialize(ENGINE_NAMESPACE::Internal::Window* window)
{

    pBackend->InitializeBackend(window, this);

    auto depthDesc = nvrhi::TextureDesc()
        .setFormat(nvrhi::Format::D16)
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setIsTypeless(true)
        .setIsRenderTarget(true)
        .setInitialState(nvrhi::ResourceStates::DepthWrite)
        .setKeepInitialState(true)
        .setClearValue(nvrhi::Color(1.0f))
        .setWidth(NvBackBuffers[0]->getDesc().width)
        .setHeight(NvBackBuffers[0]->getDesc().height);

    for (int i = 0; i < MaxInFlightFrames; i++)
    {
        if (!NvBackBuffers[i])
        {
            continue;
        }

        NvDepthBuffers[i] = pDevice->createTexture(depthDesc);

        auto rtvDescription = nvrhi::FramebufferDesc();
        rtvDescription.addColorAttachment(NvBackBuffers[i].Get());
        rtvDescription.setDepthAttachment(NvDepthBuffers[i].Get());

        NvFramebufferRtvs[i] = pDevice->createFramebuffer(rtvDescription);
    }

    CopyEngine::Init();
    //MemoryPoolAllocator::Init();

}

void RendererContext::Terminate()
{
    pBackend->TerminateBackend(this);
}

void RendererContext::present(Internal::Window* window)
{

    FrameCount++;

    bool Resized = false;

    if (pBackend->RequiresResize(window))
    {

        Resized = true;

        for (int i = 0; i < swapChainPipelines.size(); i++)
        {
            auto ptr = swapChainPipelines[i];
            ptr->PipelineHandle = nullptr;
        }

    }

    pDevice->runGarbageCollection();

    pBackend->Present(window, this);

    if (Resized)
    {

        auto depthDesc = nvrhi::TextureDesc()
            .setFormat(nvrhi::Format::D16)
            .setIsTypeless(true)
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setIsRenderTarget(true)
            .setInitialState(nvrhi::ResourceStates::DepthWrite)
            .setKeepInitialState(true)
            .setClearValue(nvrhi::Color(1.0f))
            .setWidth(NvBackBuffers[0]->getDesc().width)
            .setHeight(NvBackBuffers[0]->getDesc().height);

        for (int i = 0; i < MaxInFlightFrames; i++)
        {
            if (!NvBackBuffers[i])
            {
                continue;
            }

            NvDepthBuffers[i] = pDevice->createTexture(depthDesc);

            auto rtvDescription = nvrhi::FramebufferDesc();
            rtvDescription.addColorAttachment(NvBackBuffers[i].Get());
            rtvDescription.setDepthAttachment(NvDepthBuffers[i].Get());

            NvFramebufferRtvs[i] = pDevice->createFramebuffer(rtvDescription);
        }

    }
}

void RendererContext::ImGuiInit(Internal::Window* window)
{
    ImGui_ImplSDL3_InitForD3D(window->GetHandle());
    pBackend->ImGuiInit(window);
}

void RendererContext::ImGuiBeginFrame()
{
    ImGui_ImplSDL3_NewFrame();
    pBackend->ImGuiBeginFrame();
}

void RendererContext::ImGuiEndFrame()
{
    pBackend->ImGuiEndFrame(this);
}

void RendererContext::ImGuiShutdown()
{
    pBackend->ImGuiShutdown();
    ImGui_ImplSDL3_Shutdown();
}

void RendererContext::VideoMemoryAdd(size_t size)
{
    if (size <= 4096)
    {
        s_Context->mUsedVideoMemory.fetch_add(4096);
        return;
    }
    s_Context->mUsedVideoMemory.fetch_add(AlignTo(size, 65535ULL));
}

void RendererContext::VideoMemorySub(size_t size)
{
    if (size <= 4096)
    {
        s_Context->mUsedVideoMemory.fetch_sub(4096);
        return;
    }
    s_Context->mUsedVideoMemory.fetch_sub(AlignTo(size, 65535ULL));
}

size_t RendererContext::GetSizeForFormat(uint32_t width, uint32_t height, uint32_t format)
{
    return width * height * g_ImageBPPTable[format];
}

void RendererContext::set_vsync(bool vsync)
{
    m_VsyncState = vsync;
}

bool RendererContext::IsCapabilitySupported(RendererCapability capability)
{
    return true;
}

GraphicsDeviceProperties RendererContext::GetGraphicsDeviceProperties()
{
    auto prop = pBackend->GetGraphicsDeviceProperties();
    prop.UsedVideoMemory = s_Context->mUsedVideoMemory.load();
    return prop;
}

void RendererContext::BlitTexture(void* src, void* dst, int slice)
{
    
}

void RendererContext::RegisterBackBufferPipeline(GraphicsPipeline* pField)
{
    swapChainPipelines.push_back(pField);
}

void DebugCallbackLogger::message(nvrhi::MessageSeverity severity, const char* messageText)
{
    Z_WARN(messageText);
}
