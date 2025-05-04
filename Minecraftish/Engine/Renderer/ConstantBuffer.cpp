#include "ConstantBuffer.h"
#include "CopyEngine.h"

using namespace ENGINE_NAMESPACE;

Render::ConstantBuffer::ConstantBuffer(size_t size, void* defaultData)
{
	bool isStatic = defaultData;
	size_t dataSize = size;

	if (size % 16 != 0)
	{
		size = size + (16 - size % 16);
	}

	auto bufferDesc = nvrhi::BufferDesc()
		.setByteSize(size)
		.setIsConstantBuffer(true)
		.setMaxVersions(16)
		.setKeepInitialState(!isStatic)
		.setCpuAccess(RendererContext::get_api() == RendererAPI::DX11 ? nvrhi::CpuAccessMode::Write : nvrhi::CpuAccessMode::None)
		.setInitialState(isStatic ? nvrhi::ResourceStates::ConstantBuffer : nvrhi::ResourceStates::CopyDest)
		.setDebugName("Constant Buffer");
	Handle = RendererContext::GetDevice()->createBuffer(bufferDesc);
	Size = size;

	if (!defaultData)
	{
		mIsReady = true;
	}
	else
	{
		auto cmd = CopyEngine::WriteBuffer(Handle, defaultData, dataSize, &mIsReady);
		cmd.commandList->setPermanentBufferState(Handle, nvrhi::ResourceStates::ConstantBuffer);
		CopyEngine::Submit(cmd);
	}

}

Render::ConstantBuffer::~ConstantBuffer()
{
}

bool Render::ConstantBuffer::IsResourceReady()
{
	return mIsReady;
}
