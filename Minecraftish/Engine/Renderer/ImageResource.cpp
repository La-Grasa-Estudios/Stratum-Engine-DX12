#include "ImageResource.h"
#include "RendererContext.h"
#include "CopyEngine.h"

using namespace ENGINE_NAMESPACE;

Render::ImageResource::ImageResource(const ImageDescription& desc)
{
	ImageDesc = desc;

	auto textureDesc = nvrhi::TextureDesc();

	textureDesc.setWidth(desc.Width);
	textureDesc.setHeight(desc.Height);
	textureDesc.setMipLevels(desc.MipLevels);
	textureDesc.setArraySize(desc.ArraySize);

	textureDesc.setFormat(FormatUtil::ConvertEngineFormatToNV(desc.Format));

	textureDesc.setSampleCount(1);
	textureDesc.setSampleQuality(0);

	textureDesc.setIsRenderTarget(desc.AllowFramebufferUsage);

	textureDesc.setInitialState(nvrhi::ResourceStates::Common);
	textureDesc.setKeepInitialState(false);

	textureDesc.setDebugName("Image");

	if (textureDesc.isRenderTarget)
	{
		textureDesc.setDebugName("Image Render Target");

		textureDesc.setClearValue(nvrhi::Color(desc.ClearValue.r, desc.ClearValue.g, desc.ClearValue.b, desc.ClearValue.a));
		textureDesc.setUseClearValue(true);

		if (desc.IsDepthFormat())
		{
			textureDesc.setIsTypeless(true);
			textureDesc.setInitialState(nvrhi::ResourceStates::DepthWrite);
			textureDesc.setDebugName("Image Depth Target");
		}
		else
		{
			textureDesc.setInitialState(nvrhi::ResourceStates::RenderTarget);
		}
	}

	textureDesc.setDimension(nvrhi::TextureDimension::Texture2D);

	if (textureDesc.arraySize > 1)
	{
		textureDesc.setDimension(nvrhi::TextureDimension::Texture2DArray);
	}

	if (desc.Type == ImageType::CUBEMAP_TEXTURE)
	{
		assert(desc.ArraySize % 6 == 0);
		textureDesc.setDimension(desc.ArraySize > 6 ? nvrhi::TextureDimension::TextureCube : nvrhi::TextureDimension::TextureCubeArray);
	}

	textureDesc.setIsUAV(desc.AllowComputeResourceUsage);

	if (!desc.Immutable)
	{
		textureDesc.setKeepInitialState(true);
	}

	Handle = RendererContext::GetDevice()->createTexture(textureDesc);

	if (!desc.DefaultData.empty())
	{
		auto cmd = CopyEngine::WriteTexture(Handle, &mResourceReady);

		cmd.commandList->setTextureState(Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);

		for (int i = 0; i < desc.DefaultData.size() && i < desc.MipLevels; i++)
		{
			ImageResourceData rsc = desc.DefaultData[i];
			cmd.commandList->writeTexture(Handle, 0, i, rsc.pSysMem, rsc.MemPitch);
		}

		if (desc.Immutable)
		{
			cmd.commandList->setPermanentTextureState(Handle, nvrhi::ResourceStates::ShaderResource);
			cmd.commandList->commitBarriers();
		}

		CopyEngine::Submit(cmd);

	}
	else
	{
		mResourceReady = true;
	}

	size_t size = 0;
	for (int i = 0; i < desc.MipLevels; i++)
	{
		size += RendererContext::GetSizeForFormat(ImageDesc.Width / (i + 1), ImageDesc.Height / (i + 1), (uint32_t)ImageDesc.Format);
	}
	RendererContext::VideoMemoryAdd(size);
}

Render::ImageResource::~ImageResource()
{
	size_t size = 0;
	for (int i = 0; i < ImageDesc.MipLevels; i++)
	{
		size += RendererContext::GetSizeForFormat(ImageDesc.Width / (i + 1), ImageDesc.Height / (i + 1), (uint32_t)ImageDesc.Format);
	}
	RendererContext::VideoMemorySub(size);
}

glm::ivec2 Render::ImageResource::GetSize()
{
	return glm::ivec2(ImageDesc.Width, ImageDesc.Height);
}

bool Render::ImageResource::IsResourceReady()
{
	return mResourceReady;
}
