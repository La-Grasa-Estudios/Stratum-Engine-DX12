#include "TextureSampler.h"

#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;
using namespace Render;

Render::TextureSampler::TextureSampler(Ref<ImageResource> image)
{
	SetImage(image);
}

Render::TextureSampler::TextureSampler(Ref<Texture3D> t3d)
{
	m_TextureRef = t3d;

	
}

void Render::TextureSampler::SetFilter(TextureFilterMode mode)
{
	
}

void Render::TextureSampler::SetWrapMode(TextureWrapMode mode)
{
	
}

void Render::TextureSampler::SetLod(float minLod, float maxLod)
{
	
}

glm::vec2 Render::TextureSampler::GetLod()
{
	return { 0.0f, 0.0f };
}

void* Render::TextureSampler::GetRendererData()
{
	return NULL;
}

void Render::TextureSampler::SetImage(Ref<ImageResource> image)
{
	m_ImageRef = image;

}

Ref<ImageResource> Render::TextureSampler::GetImage()
{
	return m_ImageRef;
}

Ref<Texture3D> Render::TextureSampler::GetTexture()
{
	return m_TextureRef;
}

Render::TextureSampler::~TextureSampler()
{
	m_ImageRef = NULL;
	m_TextureRef = NULL;
}
