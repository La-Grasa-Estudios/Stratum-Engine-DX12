#pragma once

#include "znmsp.h"
#include "Renderer/Post/PostProcessingPass.h"

BEGIN_ENGINE

namespace Render
{
	class BloomPass : public PostProcessingPass
	{

	public:

		BloomPass();
		~BloomPass();

		std::vector<PassDependency> GetDependencies() override;
		void GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names) override;
		void SetInput(Ref<ImageResource> input, const std::string& name);

		void Init(glm::ivec2 Resolution) override;

		void Render(const PostProcessingParameters& parameters) override;

		Ref<Render::ComputePipeline> BloomFilterShader;
		Ref<Render::GraphicsPipeline> BloomDownsampleShader;
		Ref<Render::GraphicsPipeline> BloomUpsampleShader;

		static constexpr uint32_t bloomMipCount = 6;
		Ref<Render::ImageResource> BloomMipChainImageResource[bloomMipCount];
		Ref<Render::Framebuffer> BloomMipRt[bloomMipCount];

		Ref<Render::TextureSampler> BloomSampler;

		Ref<ImageResource> LuminanceImage;


	};
}

END_ENGINE