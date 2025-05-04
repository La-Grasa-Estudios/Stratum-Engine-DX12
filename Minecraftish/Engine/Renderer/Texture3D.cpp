#include "Texture3D.h"

#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;

Render::Texture3D::Texture3D(const Texture3DDescription& desc)
{
	TextureDesc = desc;

	auto textureDesc = nvrhi::TextureDesc();

	textureDesc.width = desc.Width;
	textureDesc.height = desc.Height;
	textureDesc.depth = desc.Depth;
	textureDesc.mipLevels = desc.MipLevels;
	textureDesc.format = FormatUtil::ConvertEngineFormatToNV(desc.Format);
	textureDesc.setDimension(nvrhi::TextureDimension::Texture3D);

	textureDesc.setIsUAV(desc.AllowComputeResourceUsage);

	Handle = RendererContext::GetDevice()->createTexture(textureDesc);

	RendererContext::VideoMemoryAdd(RendererContext::GetSizeForFormat(TextureDesc.Width, TextureDesc.Height, (uint32_t)TextureDesc.Format) * TextureDesc.Depth);

}

Render::Texture3D::~Texture3D()
{
	RendererContext::VideoMemorySub(RendererContext::GetSizeForFormat(TextureDesc.Width, TextureDesc.Height, (uint32_t)TextureDesc.Format) * TextureDesc.Depth);
}

glm::ivec3 Render::Texture3D::GetSize()
{
	return glm::ivec3(TextureDesc.Width, TextureDesc.Height, TextureDesc.Depth);
}
