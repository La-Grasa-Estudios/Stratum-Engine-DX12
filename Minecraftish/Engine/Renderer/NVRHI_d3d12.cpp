#include "NVRHI_d3d12.h"

using namespace ENGINE_NAMESPACE;

#include "DX12/GlobalSharedData.h"
#include "DX12/DX12Utils.h"
#include "DX12/directx/d3dx12.h"

#include "Core/Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_syswm.h>
#include <nvrhi/d3d12.h>

#include "Thirdparty/imgui/imgui_impl_dx12.h"


// Straight from https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp
// I'm Lazy :D
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        IM_ASSERT(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n - 1);
    }
    void Destroy()
    {
        Heap = nullptr;
        FreeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;

void Render::BackendInitializerD3D12::InitializeBackend(Internal::Window* pWindow, RendererContext* pContext)
{
    dxSharedData = new DX12::DX12Data();

    dxSharedData->FrameCount = 0;

    SDL_SysWMinfo wmInfo{};
    SDL_version sdlver;
    SDL_VERSION(&sdlver);
    wmInfo.version = SDL_GetVersion(&sdlver);
    if (SDL_GetWindowWMInfo(pWindow->GetHandle(), &wmInfo, SDL_SYSWM_CURRENT_VERSION) != 0) {
        printf("Cant get native window handle: %s\n", SDL_GetError());
    }
    dxSharedData->hWnd = wmInfo.info.win.window;

    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

        // Check to see if the adapter can create a D3D12 device without actually 
        // creating it. The adapter with the largest dedicated video memory
        // is favored.
        if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) &&
            dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
        {
            maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
            dxgiAdapter1.As(&dxgiAdapter4);
        }
    }

    if (!dxgiAdapter4)
    {
        exit(-1);
    }

    dxSharedData->Adapter = dxgiAdapter4;

#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
#endif

    D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&dxSharedData->Device));

    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(dxSharedData->Device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
    }
#endif

    dxSharedData->CommandQueue = DX12::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    dxSharedData->CopyQueue = DX12::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.errorCB = &pContext->mCallbackLogger;
    deviceDesc.pDevice = dxSharedData->Device.Get();
    deviceDesc.pGraphicsCommandQueue = dxSharedData->CommandQueue.Get();
    deviceDesc.pCopyCommandQueue = dxSharedData->CopyQueue.Get();

    pContext->pDevice = nvrhi::d3d12::createDevice(deviceDesc);

    ComPtr<IDXGISwapChain4> dxgiSwapChain4;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = pWindow->GetWidth();
    swapChainDesc.Height = pWindow->GetHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = DX12::s_MaxInFlightFrames;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    dxSharedData->WindowSize = { swapChainDesc.Width, swapChainDesc.Height };

    ComPtr<IDXGISwapChain1> dxgiSwapChain1;

    dxgiFactory->CreateSwapChainForHwnd(dxSharedData->CommandQueue.Get(), dxSharedData->hWnd, &swapChainDesc, NULL, NULL, &dxgiSwapChain1);

    dxgiSwapChain1.As(&dxgiSwapChain4);

    dxSharedData->SwapChain = dxgiSwapChain4;

    dxgiFactory->MakeWindowAssociation(dxSharedData->hWnd, DXGI_MWA_NO_ALT_ENTER);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};

    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = 1024;

    dxSharedData->RtvDescriptorHeap = std::make_shared<DX12::DynamicDescriptorHeap>(desc);

    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    dxSharedData->DsvDescriptorHeap = std::make_shared<DX12::DynamicDescriptorHeap>(desc);

    for (int i = 0; i < DX12::s_MaxInFlightFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        dxgiSwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

        CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv = dxSharedData->RtvDescriptorHeap->PopDescriptorHandle();

        dxSharedData->Device->CreateRenderTargetView(backBuffer.Get(), nullptr, Rtv);

        dxSharedData->BackBufferRtvs[i] = Rtv;
        dxSharedData->BackBuffers[i] = backBuffer;

        auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setWidth(swapChainDesc.Width)
            .setHeight(swapChainDesc.Height)
            .setIsRenderTarget(true)
            .setInitialState(nvrhi::ResourceStates::Present)
            .setKeepInitialState(true)
            .setDebugName("Swap Chain Image");

        pContext->NvBackBuffers[i] = pContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, dxSharedData->BackBuffers[i].Get(), textureDesc);

        dxSharedData->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&dxSharedData->CommandAllocators[i]));

        dxSharedData->BackBufferResourceStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }

    DX12::DX12Data::g_SharedData;

    dxSharedData->BackBufferFence = std::make_shared<DX12::DX12Fence>();

    dxSharedData->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dxSharedData->CommandAllocators[DX12::s_MaxInFlightFrames - 1].Get(), NULL, IID_PPV_ARGS(&dxSharedData->CommandList));
    dxSharedData->CommandList->Close();

    mCommandList = pContext->pDevice->createCommandList();

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 4096;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (dxSharedData->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return;
        g_pd3dSrvDescHeapAlloc.Create(dxSharedData->Device.Get(), g_pd3dSrvDescHeap);
    }
}

void Render::BackendInitializerD3D12::TerminateBackend(RendererContext* pContext)
{
    uint64_t val = dxSharedData->BackBufferFence->Signal(dxSharedData->CommandQueue.Get());
    dxSharedData->BackBufferFence->WaitForSingleObject(val);

    pContext->pDevice->waitForIdle();
    pContext->pDevice->runGarbageCollection();

    for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
    {
        dxSharedData->BackBuffers[i].Reset();
        pContext->NvFramebufferRtvs[i] = nullptr;
        pContext->NvBackBuffers[i] = nullptr;
        dxSharedData->CommandAllocators[i]->Release();
    }

    pContext->pDevice = nullptr;

    g_pd3dSrvDescHeap->Release();

    dxSharedData->RtvDescriptorHeap = NULL;
    dxSharedData->BackBufferFence = NULL;
    dxSharedData->CommandList->Release();
    dxSharedData->CommandQueue->Release();
    dxSharedData->CopyQueue->Release();
    dxSharedData->SwapChain->Release();
    dxSharedData->Adapter->Release();
    dxSharedData->Device->Release();
}

void Render::BackendInitializerD3D12::Present(Internal::Window* pWindow, RendererContext* pContext)
{
    uint32_t frameIndex = dxSharedData->FrameIndex;

    uint32_t flags = pContext->m_VsyncState ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    dxSharedData->SwapChain->Present(pContext->m_VsyncState, flags);
    dxSharedData->BackBufferFence->Signal(dxSharedData->CommandQueue.Get());

    dxSharedData->FrameIndex = dxSharedData->SwapChain->GetCurrentBackBufferIndex();
    pContext->FrameIndex = dxSharedData->FrameIndex;

    dxSharedData->BackBufferFence->WaitForSingleObject();

    glm::ivec2 size = pWindow->GetFramebuffer()->GetSize();

    if (RequiresResize(pWindow))
    {

        dxSharedData->WindowSize = size;

        uint64_t val = dxSharedData->BackBufferFence->Signal(dxSharedData->CommandQueue.Get());
        dxSharedData->BackBufferFence->WaitForSingleObject(val);

        pContext->pDevice->waitForIdle();
        pContext->pDevice->runGarbageCollection();

        dxSharedData->BackBufferFence->ResetValues();

        for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
        {
            dxSharedData->BackBuffers[i].Reset();
            dxSharedData->RtvDescriptorHeap->PushDescriptorHandle(dxSharedData->BackBufferRtvs[i]);
            pContext->NvFramebufferRtvs[i] = nullptr;
            pContext->NvBackBuffers[i] = nullptr;
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        dxSharedData->SwapChain->GetDesc(&swapChainDesc);
        dxSharedData->SwapChain->ResizeBuffers(DX12::s_MaxInFlightFrames, size.x, size.y,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);

        dxSharedData->FrameIndex = dxSharedData->SwapChain->GetCurrentBackBufferIndex();
        pContext->FrameIndex = dxSharedData->FrameIndex;

        for (int i = 0; i < DX12::s_MaxInFlightFrames; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            dxSharedData->SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

            CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv = dxSharedData->RtvDescriptorHeap->PopDescriptorHandle();

            dxSharedData->Device->CreateRenderTargetView(backBuffer.Get(), nullptr, Rtv);

            dxSharedData->BackBufferRtvs[i] = Rtv;
            dxSharedData->BackBuffers[i] = backBuffer;

            auto textureDesc = nvrhi::TextureDesc()
                .setDimension(nvrhi::TextureDimension::Texture2D)
                .setFormat(nvrhi::Format::RGBA8_UNORM)
                .setWidth(size.x)
                .setHeight(size.y)
                .setIsRenderTarget(true)
                .setInitialState(nvrhi::ResourceStates::Present)
                .setKeepInitialState(true)
                .setDebugName("Swap Chain Image");

            pContext->NvBackBuffers[i] = pContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, dxSharedData->BackBuffers[i].Get(), textureDesc);
            auto rtvDescription = nvrhi::FramebufferDesc();
            rtvDescription.addColorAttachment(pContext->NvBackBuffers[i].Get());

            pContext->NvFramebufferRtvs[i] = pContext->pDevice->createFramebuffer(rtvDescription);
            dxSharedData->BackBufferResourceStates[i] = D3D12_RESOURCE_STATE_PRESENT;
        }

    }

    dxSharedData->FrameCount += 1;
}

bool Render::BackendInitializerD3D12::RequiresResize(Internal::Window* pWindow)
{
    glm::ivec2 size = pWindow->GetFramebuffer()->GetSize();
    return size != dxSharedData->WindowSize;
}

void Render::BackendInitializerD3D12::ImGuiInit(Internal::Window* window)
{
    ImGui_ImplDX12_InitInfo initInfo{};

    initInfo.Device = dxSharedData->Device.Get();
    initInfo.CommandQueue = dxSharedData->CommandQueue.Get();
    initInfo.NumFramesInFlight = DX12::s_MaxInFlightFrames;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_D16_UNORM;
    initInfo.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };

    ImGui_ImplDX12_Init(&initInfo);
}

void Render::BackendInitializerD3D12::ImGuiBeginFrame()
{
    ImGui_ImplDX12_NewFrame();
}

void Render::BackendInitializerD3D12::ImGuiEndFrame(RendererContext* pContext)
{

    mCommandList->open();

    ComPtr<ID3D12Resource> backBuffer;
    dxSharedData->SwapChain->GetBuffer(dxSharedData->FrameIndex, IID_PPV_ARGS(&backBuffer));

    D3D12_RESOURCE_BARRIER barrier{};

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

    barrier.Transition.pResource = backBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    auto commandList = (ID3D12GraphicsCommandList*)(mCommandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList));

    commandList->ResourceBarrier(1, &barrier);

    commandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
    commandList->OMSetRenderTargets(1, &dxSharedData->BackBufferRtvs[dxSharedData->FrameIndex], FALSE, nullptr);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    commandList->ResourceBarrier(1, &barrier);

    mCommandList->close();

    pContext->pDevice->executeCommandList(mCommandList);

}

void Render::BackendInitializerD3D12::ImGuiShutdown()
{

}

Render::GraphicsDeviceProperties Render::BackendInitializerD3D12::GetGraphicsDeviceProperties()
{
    size_t converted = 0;
    char description[128];
    memset(description, 0, sizeof(description));
    DXGI_ADAPTER_DESC3 desc;
    dxSharedData->Adapter->GetDesc3(&desc);
    wcstombs_s(&converted, description, sizeof(description), desc.Description, sizeof(desc.Description));

    ENGINE_NAMESPACE::Render::GraphicsDeviceProperties properties
    {
        .Description = description,
        .DedicatedVideoMemory = desc.DedicatedVideoMemory,
        .SharedVideoMemory = desc.SharedSystemMemory,
        .UsedVideoMemory = 0,
    };

    return properties;
}
