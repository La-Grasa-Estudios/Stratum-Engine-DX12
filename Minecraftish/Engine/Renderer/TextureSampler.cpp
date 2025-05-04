#include "TextureSampler.h"

#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;
using namespace Render;

Render::TextureSampler::TextureSampler(const TextureSamplerDescription& desc)
{
	nvrhi::SamplerDesc samplerDesc{};

	samplerDesc.setAllAddressModes((nvrhi::SamplerAddressMode)desc.AddressMode);
	samplerDesc.setMipBias(desc.MipBias);
	samplerDesc.setAllFilters(desc.Filter != TextureFilterMode::POINT);
	
	if (desc.Filter == TextureFilterMode::ANISOTROPIC)
		samplerDesc.setMaxAnisotropy(desc.AnisoLevel);

	Handle = RendererContext::GetDevice()->createSampler(samplerDesc);
}

Render::TextureSampler::~TextureSampler()
{
}
