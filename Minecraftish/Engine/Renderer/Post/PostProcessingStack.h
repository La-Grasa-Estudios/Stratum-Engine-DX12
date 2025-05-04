#pragma once

#include "PostProcessingCommon.h"
#include "PostProcessingPass.h"

#include "Renderer/GraphicsCommandBuffer.h"

#include <unordered_map>

BEGIN_ENGINE

namespace Render
{

	class PostProcessingStack
	{
	public:
		PostProcessingStack();
		void Clear();
		void RegisterPass(Ref<PostProcessingPass> pass);
		void Init(glm::ivec2 Resolution);
		void Render(PostProcessingParameters& parameters);
	private:
		void RegisterOutput(Ref<ImageResource> output, const std::string& name);
		void Sort(glm::ivec2 Resolution);
		std::unordered_map<std::string, Ref<ImageResource>> m_Outputs;
		std::vector<Ref<PostProcessingPass>> m_PassesUnsorted;
		std::vector<Ref<PostProcessingPass>> m_Passes;
	};
}

END_ENGINE