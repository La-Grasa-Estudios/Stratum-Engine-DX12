#include "NVRHI_d3d11.h"

#include "Core/Window.h"

#include <d3d11.h>
#include <dxgi.h>
#include <nvrhi/d3d11.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_syswm.h>
#include <comdef.h>

using namespace ENGINE_NAMESPACE;

ID3D11Device* pDevice;
ID3D11DeviceContext* pContext;
IDXGISwapChain* pSwapChain = NULL;

ID3D11RasterizerState* pFillRasterizerState;
ID3D11RasterizerState* pLineRasterizerState;

ID3D11RenderTargetView* pRenderTargetView;
ID3D11Texture2D* pDepthStencil;
ID3D11DepthStencilView* pDepthStencilView;

D3D11_TEXTURE2D_DESC pRenderTargetDesc;
D3D11_TEXTURE2D_DESC pStencilDesc;
D3D11_VIEWPORT pViewport;
DXGI_ADAPTER_DESC Adapter;
HWND hWnd;

glm::ivec2 d3d11_WindowSize;

void Render::BackendInitializerD3D11::InitializeBackend(Internal::Window* pWindow, RendererContext* pEngineContext)
{
	SDL_SysWMinfo wmInfo{};
	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	wmInfo.version = SDL_GetVersion(&sdlver);
	if (SDL_GetWindowWMInfo(pWindow->GetHandle(), &wmInfo, SDL_SYSWM_CURRENT_VERSION) != 0) {
		printf("Cant get native window handle: %s\n", SDL_GetError());
	}
	hWnd = wmInfo.info.win.window;

	UINT CreateDeviceFlags = 0;

	D3D_FEATURE_LEVEL feature_level;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	DXGI_SWAP_EFFECT swapEffects = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = Render::MaxInFlightFrames;
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = swapEffects;

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
	};

	UINT flags = 0;
	//flags |= D3D11_CREATE_DEVICE_DEBUG;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		flags,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&pSwapChain,
		&pDevice,
		&feature_level,
		&pContext);
	if (FAILED(hr)) {

		std::string error = "A D3D11-compatible GPU (Feature Level 11.1, Shader Model 5.0) is required to run the engine. HR: ";

		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();

		std::string fcid;

#ifdef UNICODE
		int wlen = lstrlenW(errMsg);
		int len = WideCharToMultiByte(CP_ACP, 0, errMsg, wlen, NULL, 0, NULL, NULL);
		fcid.resize(len);
		WideCharToMultiByte(CP_ACP, 0, errMsg, wlen, &fcid[0], len, NULL, NULL);
#else
		fcid = errMsg;
#endif

		error.append(fcid);

		MessageBoxA(NULL, error.c_str(), "Zircon fatal error", MB_OK | MB_ICONERROR);
		exit(-1);
	}

#ifdef TRACY_ENABLE
	g_d3d11TracyContext = TracyD3D11Context(pDevice, pContext);
#endif

	ID3D11Texture2D* pBackBuffer;

	if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(LPVOID*)&pBackBuffer)))
	{
		MessageBoxA(NULL, "Failed to create back buffer!", "Zircon fatal error", MB_OK);
		exit(-1);
	}

	if (FAILED(pDevice->CreateRenderTargetView(pBackBuffer, NULL,
		&pRenderTargetView)))
	{
		MessageBoxA(NULL, "Failed to create render target!", "Zircon fatal error", MB_OK);
		exit(-1);
	}

	ID3D10Multithread* pMP = 0;
	pContext->QueryInterface<ID3D10Multithread>(&pMP);

	pMP->SetMultithreadProtected(TRUE);

	D3D11_RASTERIZER_DESC rasterizerState;
	ZeroMemory(&rasterizerState, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerState.CullMode = D3D11_CULL_BACK;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.FrontCounterClockwise = true;

	pDevice->CreateRasterizerState(&rasterizerState, &pFillRasterizerState);

	rasterizerState.CullMode = D3D11_CULL_NONE;
	rasterizerState.FillMode = D3D11_FILL_WIREFRAME;

	pDevice->CreateRasterizerState(&rasterizerState, &pLineRasterizerState);

	pContext->RSSetState(pFillRasterizerState);

	pBackBuffer->Release();

	RECT winRect;
	GetClientRect(hWnd, &winRect);
	ZeroMemory(&pViewport, sizeof(D3D11_VIEWPORT));
	pViewport.TopLeftX = 0;
	pViewport.TopLeftY = 0;
	pViewport.Width = (float)winRect.right;
	pViewport.Height = (float)winRect.bottom;
	pViewport.MinDepth = 0.0f;
	pViewport.MaxDepth = 1.0f;
	pContext->RSSetViewports(1, &pViewport);

	d3d11_WindowSize = glm::ivec2(pViewport.Width, pViewport.Height);

	ZeroMemory(&pRenderTargetDesc, sizeof(D3D11_TEXTURE2D_DESC));
	ZeroMemory(&pStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));

	pStencilDesc.Width = winRect.right;
	pStencilDesc.Height = winRect.bottom;
	pStencilDesc.MipLevels = 1;
	pStencilDesc.ArraySize = 1;
	pStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pStencilDesc.SampleDesc.Count = 1;
	pStencilDesc.SampleDesc.Quality = 0;
	pStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	pStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	pStencilDesc.CPUAccessFlags = 0;
	pStencilDesc.MiscFlags = 0;

	pDevice->CreateTexture2D(&pStencilDesc, NULL, &pDepthStencil);
	pDevice->CreateDepthStencilView(pDepthStencil, NULL, &pDepthStencilView);

	auto textureDesc = nvrhi::TextureDesc()
		.setDimension(nvrhi::TextureDimension::Texture2D)
		.setFormat(nvrhi::Format::RGBA8_UNORM)
		.setWidth(pStencilDesc.Width)
		.setHeight(pStencilDesc.Height)
		.setIsRenderTarget(true)
		.setInitialState(nvrhi::ResourceStates::Present)
		.setKeepInitialState(true)
		.setDebugName("Swap Chain Image");

	nvrhi::d3d11::DeviceDesc deviceDesc;
	deviceDesc.messageCallback = &pEngineContext->mCallbackLogger;
	deviceDesc.context = pContext;

	pEngineContext->pDevice = nvrhi::d3d11::createDevice(deviceDesc);
	pEngineContext->NvBackBuffers[0] = pEngineContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D11_Resource, pBackBuffer, textureDesc);

	auto rtvDesc = nvrhi::FramebufferDesc()
		.addColorAttachment(pEngineContext->NvBackBuffers[0]);

	pEngineContext->NvFramebufferRtvs[0] = pEngineContext->pDevice->createFramebuffer(rtvDesc);

}

void Render::BackendInitializerD3D11::Present(Internal::Window* pWindow, RendererContext* pEngineContext)
{
	pSwapChain->Present(pEngineContext->m_VsyncState, 0);

	if (RequiresResize(pWindow))
	{
		pContext->ClearState();
		pEngineContext->NvBackBuffers[0].Reset();
		pEngineContext->NvFramebufferRtvs[0].Reset();

		pRenderTargetView->Release();

		HRESULT hr;

		RECT winRect;
		GetClientRect(hWnd, &winRect);
		ZeroMemory(&pViewport, sizeof(D3D11_VIEWPORT));
		pViewport.TopLeftX = 0;
		pViewport.TopLeftY = 0;
		pViewport.Width = (float)winRect.right;
		pViewport.Height = (float)winRect.bottom;
		pViewport.MinDepth = 0.0f;
		pViewport.MaxDepth = 1.0f;

		hr = pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		// Perform error handling here!
		// Get buffer and create a render-target-view.
		ID3D11Texture2D* pBuffer;
		hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
			(void**)&pBuffer);
		// Perform error handling here!

		hr = pDevice->CreateRenderTargetView(pBuffer, NULL,
			&pRenderTargetView);
		// Perform error handling here!
		pBuffer->Release();

		pStencilDesc.Width = winRect.right;
		pStencilDesc.Height = winRect.bottom;

		pDepthStencil->Release();
		pDepthStencilView->Release();

		hr = pDevice->CreateTexture2D(&pStencilDesc, NULL, &pDepthStencil);
		hr = pDevice->CreateDepthStencilView(pDepthStencil, NULL, &pDepthStencilView);

		auto textureDesc = nvrhi::TextureDesc()
			.setDimension(nvrhi::TextureDimension::Texture2D)
			.setFormat(nvrhi::Format::RGBA8_UNORM)
			.setWidth(pStencilDesc.Width)
			.setHeight(pStencilDesc.Height)
			.setIsRenderTarget(true)
			.setInitialState(nvrhi::ResourceStates::Present)
			.setKeepInitialState(true)
			.setDebugName("Swap Chain Image");

		pEngineContext->NvBackBuffers[0] = pEngineContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D11_Resource, pBuffer, textureDesc);

		auto rtvDesc = nvrhi::FramebufferDesc()
			.addColorAttachment(pEngineContext->NvBackBuffers[0]);

		pEngineContext->NvFramebufferRtvs[0] = pEngineContext->pDevice->createFramebuffer(rtvDesc);

		d3d11_WindowSize = glm::ivec2(pViewport.Width, pViewport.Height);
	}
}

bool Render::BackendInitializerD3D11::RequiresResize(Internal::Window* pWindow)
{
	glm::ivec2 size = pWindow->GetFramebuffer()->GetSize();
	return size != d3d11_WindowSize;
}
