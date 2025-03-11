#include "CommandBuffer.h"

#include "DX12/GlobalSharedData.h"
#include "DX12/DX12Utils.h"

#include "Core/Logger.h"
#include "Core/Timer.h"

#include <thread>

using namespace ENGINE_NAMESPACE;

#define listData reinterpret_cast<DX12CmdListData*>(NativeData)
#define bufferData(NativeData) reinterpret_cast<DX12::DX12BufferNativeData*>(NativeData)

struct DX12CmdListData
{
	ComPtr<ID3D12GraphicsCommandList> CommandList;
	ComPtr<ID3D12CommandAllocator> CommandAllocator[DX12::s_MaxInFlightFrames];
};

void TransitionResource(ID3D12GraphicsCommandList* cmd, ID3D12Resource* rsc, D3D12_RESOURCE_STATES* before, D3D12_RESOURCE_STATES after)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(rsc, *before, after);
	cmd->ResourceBarrier(1, &barrier);
	*before = after;
}

Render::CommandBuffer::CommandBuffer(uint32_t flags)
{
	NativeData = new DX12CmdListData();

	auto device = dxSharedData->Device.Get();

	for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
	{
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&listData->CommandAllocator[i]));
	}

	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, listData->CommandAllocator[DX12::s_MaxInFlightFrames - 1].Get(), NULL, IID_PPV_ARGS(&listData->CommandList));
	listData->CommandList->Close();
}

Render::CommandBuffer::~CommandBuffer()
{
	delete listData;
}

void Render::CommandBuffer::Begin()
{
	uint32_t frameIndex = dxSharedData->FrameIndex;
	listData->CommandAllocator[frameIndex]->Reset();
	listData->CommandList->Reset(listData->CommandAllocator[frameIndex].Get(), NULL);

	listData->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Render::CommandBuffer::End()
{
	listData->CommandList->Close();
}

void Render::CommandBuffer::Submit()
{
	ID3D12CommandList* const commandLists[] = {
			listData->CommandList.Get()
	};
	dxSharedData->CommandQueue->ExecuteCommandLists(1, commandLists);
}

void Render::CommandBuffer::SetPipeline(GraphicsPipeline* pipeline)
{
	auto ptr = pipeline->As<DX12::DX12PsoNativeData>();

	listData->CommandList->SetPipelineState(ptr->PipelineState.Get());
	listData->CommandList->SetGraphicsRootSignature(ptr->RootSignature.Get());

	pCurrentPipeline = pipeline;
}

void Render::CommandBuffer::SetVertexBuffer(VertexBuffer* buffer, uint32_t slot)
{
#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!buffer->GetBuffer()->IsResourceReady())
	{
		std::this_thread::yield();
#ifdef _DEBUG
		DidWait = true;
#endif
	}

#ifdef _DEBUG
	if (DidWait)
	{
		Z_WARN("CPU Thread stalled for {}ms waiting for resource to be in a ready state! Resource Pointer: {}", timer.GetMillis(), (void*)buffer);
	}
#endif

	if (buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->ResourceState != D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	{
		TransitionResource(listData->CommandList.Get(), buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->resource.Get(),
			&buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->ResourceState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	}

	listData->CommandList->IASetVertexBuffers(slot, 1, &buffer->As<DX12::DX12VertexBufferNativeData>()->BufferView);
}

void Render::CommandBuffer::SetIndexBuffer(IndexBuffer* buffer)
{
#ifdef _DEBUG
	Timer timer;
	bool DidWait = false;
#endif
	while (!buffer->GetBuffer()->IsResourceReady())
	{
		std::this_thread::yield();
#ifdef _DEBUG
		DidWait = true;
#endif
	}

#ifdef _DEBUG
	if (DidWait)
	{
		Z_WARN("CPU Thread stalled for {}ms waiting for resource to be in a ready state! Resource Pointer: {}", timer.GetMillis(), (void*)buffer);
	}
#endif
	if (buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->ResourceState != D3D12_RESOURCE_STATE_INDEX_BUFFER)
	{
		TransitionResource(listData->CommandList.Get(), buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->resource.Get(),
			&buffer->GetBuffer()->As<DX12::DX12BufferNativeData>()->ResourceState, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}

	listData->CommandList->IASetIndexBuffer(&buffer->As<DX12::DX12IndexBufferNativeData>()->BufferView);
}

void Render::CommandBuffer::SetFramebuffer(Framebuffer* framebuffer, ComputeResource** pUavs, uint32_t nbUavs)
{
	if (framebuffer->IsWindowFramebuffer())
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dxSharedData->BackBuffers[dxSharedData->FrameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		listData->CommandList->ResourceBarrier(1, &barrier);
		dxSharedData->BackBufferResourceStates[dxSharedData->FrameIndex] = D3D12_RESOURCE_STATE_RENDER_TARGET;

		listData->CommandList->OMSetRenderTargets(1, &dxSharedData->BackBufferRtvs[dxSharedData->FrameIndex], FALSE, NULL);
	}
}

void Render::CommandBuffer::PushConstants(void* ptr, size_t size, uint32_t index)
{
	auto pipeline = pCurrentPipeline->As<DX12::DX12PsoNativeData>();

	listData->CommandList->SetGraphicsRoot32BitConstants(pipeline->CBIndexes[index], size / 4, ptr, 0);
}

void Render::CommandBuffer::SetViewport(Viewport* vp)
{
	D3D12_VIEWPORT viewport{};
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	viewport.Width = vp->width;
	viewport.Height = vp->height;
	viewport.TopLeftX = vp->x;
	viewport.TopLeftY = vp->y;
	listData->CommandList->RSSetViewports(1, &viewport);

	D3D12_RECT rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

	listData->CommandList->RSSetScissorRects(1, &rect);
}

void Render::CommandBuffer::Draw(uint32_t count)
{
	listData->CommandList->DrawInstanced(count, 1, 0, 0);
}

void Render::CommandBuffer::DrawIndexed(uint32_t count)
{
	listData->CommandList->DrawIndexedInstanced(count, 1, 0, 0, 0);
}

void Render::CommandBuffer::ClearBuffer(Framebuffer* framebuffer, const glm::vec4 color, float depth)
{
	if (framebuffer->IsWindowFramebuffer())
	{
		listData->CommandList->ClearRenderTargetView(dxSharedData->BackBufferRtvs[dxSharedData->FrameIndex], glm::value_ptr(color), 0, NULL);
	}
}

void Render::CommandBuffer::Barrier(ComputeResource* resource)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(dxSharedData->BackBuffers[dxSharedData->FrameIndex].Get());
	listData->CommandList->ResourceBarrier(1, &barrier);
}
