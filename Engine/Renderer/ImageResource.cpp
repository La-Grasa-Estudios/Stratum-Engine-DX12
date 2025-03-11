#include "ImageResource.h"
#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;

Render::ImageResource::ImageResource(const ImageDescription& desc)
{
	ImageDesc = desc;
	
}

void* Render::ImageResource::Map()
{
	return NULL;
}

void Render::ImageResource::Unmap()
{
	
}

glm::ivec2 Render::ImageResource::GetSize()
{
	return glm::ivec2(ImageDesc.Width, ImageDesc.Height);
}
