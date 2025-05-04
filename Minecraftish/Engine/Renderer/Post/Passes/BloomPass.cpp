#include "BloomPass.h"
#include "Outputs.h"

using namespace ENGINE_NAMESPACE;

Render::BloomPass::BloomPass()
{
	PipelineDescription shaderDesc;

	shaderDesc.ShaderPath = "shaders/post/bloom_downsample.cso";

	shaderDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(glm::ivec2)));

	shaderDesc.RasterizerState.DepthTest = false;
	shaderDesc.StencilState.DepthEnable = false;

	BloomDownsampleShader = CreateRef<GraphicsPipeline>(shaderDesc);

	shaderDesc = {};
	shaderDesc.ShaderPath = "shaders/post/bloom_upsample.cso";

	shaderDesc.BlendState.BlendStates[0].BlendOperator = BlendOp::ADD;
	shaderDesc.BlendState.BlendStates[0].SrcBlend = Blend::ONE;
	shaderDesc.BlendState.BlendStates[0].DestBlend = Blend::ONE;

	shaderDesc.RasterizerState.DepthTest = false;
	shaderDesc.StencilState.DepthEnable = false;

	BloomUpsampleShader = CreateRef<GraphicsPipeline>(shaderDesc);

	ComputePipelineDesc bloomFilterDesc{};

	bloomFilterDesc.path = "shaders/compute/compute_bloom_filter.cso";

	BloomFilterShader = CreateRef<ComputePipeline>(bloomFilterDesc);

}

Render::BloomPass::~BloomPass()
{
	for (uint32_t i = 0; i < bloomMipCount; i++)
	{
		BloomMipChainImageResource[i] = NULL;
		BloomMipRt[i] = NULL;
	}
}

std::vector<Render::PassDependency> Render::BloomPass::GetDependencies()
{
    return { { LUMINANCE_PASS_OUTPUT, false } };
}

void Render::BloomPass::GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names)
{
	outputs.push_back(BloomMipChainImageResource[0]);
	names.push_back(BLOOM_PASS_OUTPUT);
}

void Render::BloomPass::SetInput(Ref<ImageResource> input, const std::string& name)
{
	if (name.compare(LUMINANCE_PASS_OUTPUT) == 0)
	{
		LuminanceImage = input;
	}
}

void Render::BloomPass::Init(glm::ivec2 Resolution)
{

	TextureSamplerDescription samplerDesc{};

	samplerDesc.AddressMode = TextureWrapMode::CLAMP;

	BloomSampler = CreateRef<TextureSampler>(samplerDesc);

	for (uint32_t i = 0; i < bloomMipCount; i++)
	{
		ImageDescription bloomMipDesc{};

		bloomMipDesc.AllowFramebufferUsage = true;
		bloomMipDesc.AllowComputeResourceUsage = i == 0;
		bloomMipDesc.Format = ImageFormat::R11G11B10_FLOAT;
		bloomMipDesc.Width = Resolution.x / (2 << i);
		bloomMipDesc.Height = Resolution.y / (2 << i);

		BloomMipChainImageResource[i] = CreateRef<ImageResource>(bloomMipDesc);

		FramebufferDesc mipDesc{};

		mipDesc.Attachments.push_back({ BloomMipChainImageResource[i].get() });

		BloomMipRt[i] = CreateRef<Framebuffer>(mipDesc);
	}

	BloomUpsampleShader->ShaderDesc.RenderTarget = BloomMipRt[0].get();
	BloomDownsampleShader->ShaderDesc.RenderTarget = BloomMipRt[0].get();
}

void Render::BloomPass::Render(const PostProcessingParameters& parameters)
{
	parameters.cCommandBuffer->SetTextureResource(parameters.pColorSampler, 0);
	parameters.cCommandBuffer->SetComputePipeline(BloomFilterShader.get());
	parameters.cCommandBuffer->SetTextureCompute(BloomMipChainImageResource[0].get(), 0);
	parameters.cCommandBuffer->SetTextureResource(LuminanceImage.get(), 1);

	parameters.cCommandBuffer->Dispatch((int)glm::ceil(parameters.Resolution.x / 2 / 16.0f), (int)glm::ceil(parameters.Resolution.y / 2 / 16.0f), 1);

	glm::ivec2 res = parameters.Resolution;

	parameters.gCommandBuffer->SetPipeline(BloomDownsampleShader.get());
	parameters.gCommandBuffer->PushConstants(&res, sizeof(glm::ivec2));

	for (int i = 1; i < bloomMipCount; i++)
	{

		Viewport bloomMipVp{};

		bloomMipVp.width = parameters.Resolution.x / (2 << i);
		bloomMipVp.height = parameters.Resolution.y / (2 << i);

		parameters.gCommandBuffer->SetViewport(&bloomMipVp);

		parameters.gCommandBuffer->SetFramebuffer(BloomMipRt[i].get());
		parameters.gCommandBuffer->SetTextureResource(BloomMipChainImageResource[i - 1].get(), 0);

		parameters.gCommandBuffer->Draw(3);

	}

	parameters.gCommandBuffer->SetPipeline(BloomUpsampleShader.get());

	for (int i = bloomMipCount - 2; i >= 0; i--)
	{

		Viewport bloomMipVp{};

		bloomMipVp.width = parameters.Resolution.x / (2 << i);
		bloomMipVp.height = parameters.Resolution.y / (2 << i);

		parameters.gCommandBuffer->SetViewport(&bloomMipVp);

		parameters.gCommandBuffer->SetFramebuffer(BloomMipRt[i].get());
		parameters.gCommandBuffer->SetTextureResource(BloomMipChainImageResource[i + 1].get(), 0);

		parameters.gCommandBuffer->Draw(3);

	}

}
