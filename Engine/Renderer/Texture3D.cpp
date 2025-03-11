#include "Texture3D.h"

#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;

Render::Texture3D::Texture3D(const Texture3DDescription& desc)
{
	TextureDesc = desc;
	
}

void* Render::Texture3D::Map()
{
	return NULL;
}

void Render::Texture3D::Unmap()
{

}

glm::ivec3 Render::Texture3D::GetSize()
{
	return glm::ivec3(TextureDesc.Width, TextureDesc.Height, TextureDesc.Depth);
}
