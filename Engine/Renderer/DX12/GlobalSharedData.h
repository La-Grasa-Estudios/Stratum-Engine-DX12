#pragma once

#include <cstdint>

#include "directx/d3d12.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <glm/ext.hpp>
#include <memory>

using namespace Microsoft::WRL;

#include "DX12Constants.h"
#include "DX12DescriptorHeap.h"
#include "DX12Fence.h"

#define dxSharedData DX12::DX12Data::g_SharedData

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

namespace DX12
{

    struct DX12BufferNativeData
    {
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES ResourceState = D3D12_RESOURCE_STATE_COMMON;
    };

    struct DX12VertexBufferNativeData
    {
        D3D12_VERTEX_BUFFER_VIEW BufferView;
    };

    struct DX12IndexBufferNativeData
    {
        D3D12_INDEX_BUFFER_VIEW BufferView;
    };

    struct DX12PsoNativeData
    {
        ComPtr<ID3D12RootSignature> RootSignature;
        ComPtr<ID3D12PipelineState> PipelineState;
		std::vector<CD3DX12_ROOT_PARAMETER1> VsRootParameters;
		std::vector<CD3DX12_ROOT_PARAMETER1> PsRootParameters;
		std::vector<uint32_t> CBIndexes;
    };

    struct DX12Data
    {
        bool IsInitialized = false;
        HWND hWnd;
        ComPtr<ID3D12Device> Device;
        ComPtr<ID3D12CommandQueue> CommandQueue;
        ComPtr<ID3D12CommandQueue> CopyQueue;
        ComPtr<IDXGISwapChain4> SwapChain;

        ComPtr<ID3D12GraphicsCommandList> CommandList;
        ComPtr<ID3D12CommandAllocator> CommandAllocators[DX12::s_MaxInFlightFrames];

        CD3DX12_CPU_DESCRIPTOR_HANDLE BackBufferRtvs[DX12::s_MaxInFlightFrames];
        D3D12_RESOURCE_STATES BackBufferResourceStates[DX12::s_MaxInFlightFrames];
        ComPtr<ID3D12Resource> BackBuffers[DX12::s_MaxInFlightFrames];
        std::shared_ptr<DX12Fence> BackBufferFence;

        std::shared_ptr<DynamicDescriptorHeap> RtvDescriptorHeap;
        std::shared_ptr<DynamicDescriptorHeap> DsvDescriptorHeap;

        uint8_t BackBufferIndex;
        glm::uvec2 WindowedWindowSize;

        glm::ivec2 WindowSize;

        uint8_t FrameIndex = 0;
        uint64_t FrameCount = 0;

        static inline DX12Data* g_SharedData;

    };

	inline static const DXGI_FORMAT g_ImageFormatTable[] = {
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM,

	DXGI_FORMAT_R8_SNORM,
	DXGI_FORMAT_R8G8_SNORM,
	DXGI_FORMAT_R8G8B8A8_SNORM,

	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_R16G16B16A16_UNORM,

	DXGI_FORMAT_R16_SNORM,
	DXGI_FORMAT_R16G16_SNORM,
	DXGI_FORMAT_R16G16B16A16_SNORM,

	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_R16G16B16A16_FLOAT,

	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,

	DXGI_FORMAT_B5G6R5_UNORM,
	DXGI_FORMAT_R11G11B10_FLOAT,

	DXGI_FORMAT_R32_TYPELESS,
	DXGI_FORMAT_R24G8_TYPELESS,
	DXGI_FORMAT_R16_TYPELESS,

	DXGI_FORMAT_R24G8_TYPELESS,

	DXGI_FORMAT_BC7_UNORM,

	DXGI_FORMAT_B8G8R8A8_UNORM,
	};

	inline static const float g_ImageByteSizeTable[] =
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


}