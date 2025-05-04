#pragma once

#include "znmsp.h"
#include "Renderer/Post/PostProcessingPass.h"

BEGIN_ENGINE

namespace Render
{
	class TonemapPass : public PostProcessingPass
	{

	public:

		TonemapPass();

		std::vector<PassDependency> GetDependencies() override;
		void SetInput(Ref<ImageResource> input, const std::string& name);

		void Init(glm::ivec2 Resolution) override;

		void Render(const PostProcessingParameters& parameters) override;

		Render::RasterizerState PostFxRS;
		Ref<GraphicsPipeline> ToneMapShader;

		Ref<ImageResource> BloomImage;
		Ref<ImageResource> LuminanceImage;

		Ref<ImageResource> DirtLensImage;

	};
}

END_ENGINE