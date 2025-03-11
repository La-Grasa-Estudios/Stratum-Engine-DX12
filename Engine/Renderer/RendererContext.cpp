#include "RendererContext.h"

using namespace ENGINE_NAMESPACE::Render;

RendererContext* RendererContext::s_Context = NULL;

#include "Core/Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_syswm.h>

#include "DX12/GlobalSharedData.h"
#include "DX12/DX12Utils.h"

#include "DX12/directx/d3dx12.h"

#include "DX12/UploadMemoryPool.h"
#include "DX12/DX12CopyEngine.h"

RendererContext::RendererContext()
{
    dxSharedData = new DX12::DX12Data();
}

void RendererContext::pre_glfw_window(ENGINE_NAMESPACE::Internal::Window* window)
{
}

void RendererContext::initialize(ENGINE_NAMESPACE::Internal::Window* window)
{

    dxSharedData->FrameCount = 0;

    SDL_SysWMinfo wmInfo{};
    SDL_version sdlver;
    SDL_VERSION(&sdlver);
    wmInfo.version = SDL_GetVersion(&sdlver);
    if (SDL_GetWindowWMInfo(window->GetHandle(), &wmInfo, SDL_SYSWM_CURRENT_VERSION) != 0) {
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
                D3D_FEATURE_LEVEL_11_1, __uuidof(ID3D12Device), nullptr)) &&
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

#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
#endif

    D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&dxSharedData->Device));

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

    DX12::MemoryPoolAllocator::Init();
    DX12::CopyEngine::Init();

    ComPtr<IDXGISwapChain4> dxgiSwapChain4;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = window->GetWidth();
    swapChainDesc.Height = window->GetHeight();
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

        dxSharedData->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&dxSharedData->CommandAllocators[i]));

        dxSharedData->BackBufferResourceStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }
    
    dxSharedData->BackBufferFence = std::make_shared<DX12::DX12Fence>();

    dxSharedData->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dxSharedData->CommandAllocators[DX12::s_MaxInFlightFrames - 1].Get(), NULL, IID_PPV_ARGS(&dxSharedData->CommandList));
    dxSharedData->CommandList->Close();

}

void RendererContext::terminate()
{
}

void RendererContext::swap_buffers(ENGINE_NAMESPACE::Internal::Window* window)
{
    

    if (dxSharedData->BackBufferResourceStates[dxSharedData->FrameIndex] != D3D12_RESOURCE_STATE_PRESENT)
    {
        auto allocator = dxSharedData->CommandAllocators[dxSharedData->FrameIndex].Get();
        allocator->Reset();
        dxSharedData->CommandList->Reset(allocator, NULL); 
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dxSharedData->BackBuffers[dxSharedData->FrameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        dxSharedData->CommandList->ResourceBarrier(1, &barrier);
        dxSharedData->CommandList->Close();

        ID3D12CommandList* const commandLists[] =
        {
            dxSharedData->CommandList.Get(),
        };

        dxSharedData->CommandQueue->ExecuteCommandLists(1, commandLists);
    }

    uint32_t flags = m_VsyncState ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    dxSharedData->SwapChain->Present(m_VsyncState, flags);

    dxSharedData->BackBufferFence->Signal(dxSharedData->CommandQueue.Get());

    dxSharedData->FrameIndex = dxSharedData->SwapChain->GetCurrentBackBufferIndex();

    dxSharedData->BackBufferFence->WaitForSingleObject();

    glm::ivec2 size = window->GetFramebuffer()->GetSize();

    if (size != dxSharedData->WindowSize)
    {
        dxSharedData->WindowSize = size;

        uint64_t val = dxSharedData->BackBufferFence->Signal(dxSharedData->CommandQueue.Get());
        dxSharedData->BackBufferFence->WaitForSingleObject(val);

        dxSharedData->BackBufferFence->ResetValues();

        for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
        {
            dxSharedData->BackBuffers[i].Reset();
            dxSharedData->RtvDescriptorHeap->PushDescriptorHandle(dxSharedData->BackBufferRtvs[i]);
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        dxSharedData->SwapChain->GetDesc(&swapChainDesc);
        dxSharedData->SwapChain->ResizeBuffers(DX12::s_MaxInFlightFrames, size.x, size.y,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags);

        dxSharedData->FrameIndex = dxSharedData->SwapChain->GetCurrentBackBufferIndex();

        for (int i = 0; i < DX12::s_MaxInFlightFrames; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            dxSharedData->SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

            CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv = dxSharedData->RtvDescriptorHeap->PopDescriptorHandle();

            dxSharedData->Device->CreateRenderTargetView(backBuffer.Get(), nullptr, Rtv);

            dxSharedData->BackBufferRtvs[i] = Rtv;
            dxSharedData->BackBuffers[i] = backBuffer;

            dxSharedData->BackBufferResourceStates[i] = D3D12_RESOURCE_STATE_PRESENT;
        }
    }

    dxSharedData->FrameCount += 1;
}

void RendererContext::ImGuiInit(ENGINE_NAMESPACE::Internal::Window* window)
{
}

void RendererContext::ImGuiBeginFrame()
{
}

void RendererContext::ImGuiEndFrame()
{
}

void RendererContext::ImGuiShutdown()
{
}

void RendererContext::set_vsync(bool vsync)
{
    m_VsyncState = vsync;
}

void RendererContext::BlitTexture(void* src, void* dst, int slice)
{
    
}