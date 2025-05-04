#pragma once

#include "znmsp.h"
#include "Renderer/Post/PostProcessingPass.h"

BEGIN_ENGINE

namespace Render
{
	class LuminancePass : public PostProcessingPass
	{

	public:

		LuminancePass();

		void GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names) override;
		void Render(const PostProcessingParameters& parameters) override;

		Ref<ImageResource> LuminanceImage;

		Ref<ComputePipeline> LuminanceShader;
		Ref<ComputePipeline> LuminanceAverageShader;
		Ref<Buffer> LuminanceBuffer;

	};
}

END_ENGINE