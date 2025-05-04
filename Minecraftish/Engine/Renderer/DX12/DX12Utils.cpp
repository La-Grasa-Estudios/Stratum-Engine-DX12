#include "DX12Utils.h"

ComPtr<ID3D12CommandQueue> DX12::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{

    D3D12_COMMAND_QUEUE_DESC desc = {};

    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Type = type;

    ComPtr<ID3D12CommandQueue> queue;
    dxSharedData->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));

    return queue;
}

ComPtr<ID3D12DescriptorHeap> DX12::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    
    ComPtr<ID3D12DescriptorHeap> heap;
    dxSharedData->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));

    return heap;
}
