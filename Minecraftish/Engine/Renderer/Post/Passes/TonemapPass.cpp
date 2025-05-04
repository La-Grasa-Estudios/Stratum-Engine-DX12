#include "TonemapPass.h"
#include "Outputs.h"

#include "Asset/TextureLoader.h"

using namespace ENGINE_NAMESPACE;

Render::TonemapPass::TonemapPass()
{
	PostFxRS.DepthTest = false;
	PostFxRS.CullMode = RCullMode::NOT;
	PostFxRS.EnableScissor = false;

	bool IsTransparent = false;
	DirtLensImage = TextureLoader::LoadFileToImage("textures/dirt_lens.png", &IsTransparent);
}

std::vector<Render::PassDependency> Render::TonemapPass::GetDependencies()
{
	return { 
		Render::PassDependency { .name = LUMINANCE_PASS_OUTPUT, .Optional = false }, 
		Render::PassDependency{ .name = BLOOM_PASS_OUTPUT, .Optional = true } 
	};
}

void Render::TonemapPass::SetInput(Ref<ImageResource> input, const std::string& name)
{
	if (name.compare(LUMINANCE_PASS_OUTPUT) == 0)
	{
		LuminanceImage = input;
	}
	if (name.compare(BLOOM_PASS_OUTPUT) == 0)
	{
		BloomImage = input;
	}
}

void Render::TonemapPass::Init(glm::ivec2 Resolution)
{
	PipelineDescription shaderDesc;
	shaderDesc.ShaderPath = "shaders/tone_map.cso";

	shaderDesc.RequirePermutation("LUMINANCE");

	if (BloomImage)
	{
		shaderDesc.RequirePermutation("BLOOM");
	}

	shaderDesc.RasterizerState.DepthTest = false;
	shaderDesc.StencilState.DepthEnable = false;

	ToneMapShader = CreateRef<GraphicsPipeline>(shaderDesc);
}

void Render::TonemapPass::Render(const PostProcessingParameters& parameters)
{

	if (!ToneMapShader->ShaderDesc.RenderTarget)
	{
		ToneMapShader->ShaderDesc.RenderTarget = parameters.pOutputFramebuffer;
	}

	parameters.gCommandBuffer->SetTextureResource(parameters.pColorSampler, 0);
	parameters.gCommandBuffer->SetTextureResource(LuminanceImage.get(), 1);
	parameters.gCommandBuffer->SetTextureResource(BloomImage.get(), 2);
	parameters.gCommandBuffer->SetTextureResource(DirtLensImage.get(), 3);
	parameters.gCommandBuffer->SetTextureSampler(parameters.pBilinearTextureSampler, 0);

	Viewport outViewport{};
	outViewport.width = parameters.OutputResolution.x;
	outViewport.height = parameters.OutputResolution.y;

	parameters.gCommandBuffer->SetViewport(&outViewport);
	parameters.gCommandBuffer->SetFramebuffer(parameters.pOutputFramebuffer);

	parameters.gCommandBuffer->SetPipeline(ToneMapShader.get());

	parameters.gCommandBuffer->Draw(3);
}
