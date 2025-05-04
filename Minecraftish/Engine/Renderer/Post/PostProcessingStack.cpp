#include "PostProcessingStack.h"

#include "Passes/BloomPass.h"
#include "Passes/LuminancePass.h"
#include "Passes/TonemapPass.h"

using namespace ENGINE_NAMESPACE;

Render::PostProcessingStack::PostProcessingStack()
{
	Clear();
}

void Render::PostProcessingStack::Clear()
{
	m_Outputs.clear();
	m_Passes.clear();
	m_PassesUnsorted.clear();
}

void Render::PostProcessingStack::RegisterOutput(Ref<ImageResource> output, const std::string& name)
{
	m_Outputs[name] = output;
}

void Render::PostProcessingStack::RegisterPass(Ref<PostProcessingPass> pass)
{
	m_PassesUnsorted.push_back(pass);
}

void Render::PostProcessingStack::Init(glm::ivec2 Resolution)
{

	RegisterPass(CreateRef<BloomPass>());
	RegisterPass(CreateRef<LuminancePass>());
	RegisterPass(CreateRef<TonemapPass>());

	Sort(Resolution);
}

void Render::PostProcessingStack::Render(PostProcessingParameters& parameters)
{
	for (int i = 0; i < m_Passes.size(); i++)
	{
		//parameters.cCommandBuffer->ClearState();
		m_Passes[i]->Render(parameters);
	}
}

bool ContainsPass(Render::PostProcessingPass* pass, std::vector<Ref<Render::PostProcessingPass>>& list)
{
	for (int i = 0; i < list.size(); i++)
	{
		if (list[i].get() == pass)
		{
			return true;
		}
	}
	return false;
}

void Render::PostProcessingStack::Sort(glm::ivec2 Resolution)
{
	const uint32_t maxAttempts = 1024;
	uint32_t attempts = 0;
	while (m_Passes.size() != m_PassesUnsorted.size())
	{
		for (int i = 0; i < m_PassesUnsorted.size(); i++)
		{
			PostProcessingPass* pass = m_PassesUnsorted[i].get();

			if (ContainsPass(pass, m_Passes))
			{
				continue;
			}

			auto dependencies = pass->GetDependencies();

			bool hasAllDependencies = true;

			for (auto& dep : dependencies)
			{
				if (!m_Outputs.contains(dep.name) && (!dep.Optional || attempts < 512))
				{
					hasAllDependencies = false;
					break;
				}
			}

			if (hasAllDependencies)
			{
				for (auto& dep : dependencies)
				{
					pass->SetInput(m_Outputs[dep.name], dep.name);
				}
			}
			else
			{
				continue;
			}

			m_Passes.push_back(m_PassesUnsorted[i]);

			pass->Init(Resolution);

			std::vector<Ref<ImageResource>> outputs;
			std::vector<std::string> names;
			pass->GetOutputs(outputs, names);

			for (int i = 0; i < outputs.size(); i++)
			{
				RegisterOutput(outputs[i], names[i]);
			}

		}
		if (maxAttempts < attempts++)
		{
			break;
		}

	}
}
