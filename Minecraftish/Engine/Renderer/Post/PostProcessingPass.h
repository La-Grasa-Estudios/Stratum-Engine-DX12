#pragma once

#include "znmsp.h"

#include "PostProcessingCommon.h"

BEGIN_ENGINE

namespace Render
{
	class PostProcessingPass
	{
	public:
		virtual std::vector<PassDependency> GetDependencies();
		virtual void GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names);
		virtual void SetInput(Ref<ImageResource> input, const std::string& name);
		virtual void Init(glm::ivec2 Resolution);
		virtual void Render(const PostProcessingParameters& parameters);
	};
}

END_ENGINE