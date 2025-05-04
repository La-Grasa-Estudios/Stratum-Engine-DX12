#pragma once

#include "Renderer/Framebuffer.h"
#include "Renderer/ImageResource.h"

#include "Renderer/GraphicsCommandBuffer.h"
#include "Renderer/ComputeCommandBuffer.h"

BEGIN_ENGINE

namespace Render
{
	struct PostProcessingParameters
	{
		ImageResource* pColorSampler;
		ImageResource* pDepthSampler;
		ImageResource* pMotionSampler;
		Framebuffer* pOutputFramebuffer;

		TextureSampler* pBilinearTextureSampler;
		TextureSampler* pNearestTextureSampler;

		glm::ivec2 Resolution;
		glm::ivec2 OutputResolution;

		GraphicsCommandBuffer* gCommandBuffer;
		ComputeCommandBuffer* cCommandBuffer;
	};

	struct PassOutput
	{
		Ref<ImageResource> Output;
		std::string Name;
	};
	
	struct PassDependency
	{
		std::string name;
		bool Optional;
	};
}

END_ENGINE