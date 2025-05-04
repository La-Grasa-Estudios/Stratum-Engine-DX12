#include "LuminancePass.h"
#include "Outputs.h"
#include <Core/Time.h>

using namespace ENGINE_NAMESPACE;

struct LuminancePassParams
{
	glm::vec2 params;
	glm::uvec2 RtSize;
	float minLogLuminance;
	float logLumRange;
	float timeCoeff;
	float numPixels;
};

Render::LuminancePass::LuminancePass()
{
	BufferDescription bufferLumDesc{};
	bufferLumDesc.AllowComputeResourceUsage = true;
	bufferLumDesc.Size = 257 * sizeof(uint32_t);
	bufferLumDesc.Type = BufferType::STORAGE;
	bufferLumDesc.Usage = BufferUsage::DEFAULT;
	bufferLumDesc.ComputeType = BufferComputeType::RAW;

	LuminanceBuffer = CreateRef<Buffer>(bufferLumDesc);

	ImageDescription luminanceDesc{};

	luminanceDesc.AllowComputeResourceUsage = true;
	luminanceDesc.Format = ImageFormat::R16_FLOAT;
	luminanceDesc.Width = 1;
	luminanceDesc.Height = 1;

	LuminanceImage = CreateRef<ImageResource>(luminanceDesc);

	ComputePipelineDesc computeDesc{};

	computeDesc.path = "shaders/compute/compute_luminance.cso";
	computeDesc.addBindingItem(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(LuminancePassParams)));

	LuminanceShader = CreateRef<ComputePipeline>(computeDesc);

	computeDesc = {};

	computeDesc.path = "shaders/compute/compute_avg_luminance.cso";
	computeDesc.addBindingItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(0));
	computeDesc.addBindingItem(nvrhi::BindingLayoutItem::Texture_UAV(1));
	computeDesc.addBindingItem(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(LuminancePassParams)));

	LuminanceAverageShader = CreateRef<ComputePipeline>(computeDesc);

}

void Render::LuminancePass::GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names)
{
	outputs.push_back(LuminanceImage);
	names.push_back(LUMINANCE_PASS_OUTPUT);
}

void Render::LuminancePass::Render(const PostProcessingParameters& parameters)
{

	float tau = 0.6f;
	float logLumRange = 4.0f - -9.0f;
	float timeCoeff = glm::clamp<float>(1.0f - glm::exp(-Time::DeltaTime * tau), 0.0f, 1.0f);

	LuminancePassParams lumaPass
	{
		.params = { -9.0f, 1.0f / logLumRange  },
		.RtSize = parameters.Resolution,
		.minLogLuminance = -9.0f,
		.logLumRange = logLumRange,
		.timeCoeff = timeCoeff,
		.numPixels = static_cast<float>(parameters.Resolution.x / 8 * parameters.Resolution.y / 8)
	};

	parameters.cCommandBuffer->SetTextureResource(parameters.pColorSampler, 0);
	parameters.cCommandBuffer->SetBufferCompute(LuminanceBuffer.get(), 0);
	parameters.cCommandBuffer->SetTextureCompute(LuminanceImage.get(), 1);

	parameters.cCommandBuffer->SetComputePipeline(LuminanceShader.get());

	parameters.cCommandBuffer->PushConstants(&lumaPass, sizeof(LuminancePassParams));
	parameters.cCommandBuffer->Dispatch((int)glm::ceil(parameters.Resolution.x / 8.0f / 16.0f), (int)glm::ceil(parameters.Resolution.y / 8.0f / 16.0f), 1);

	parameters.cCommandBuffer->SetComputePipeline(LuminanceAverageShader.get());

	parameters.cCommandBuffer->PushConstants(&lumaPass, sizeof(LuminancePassParams));
	parameters.cCommandBuffer->Dispatch(1, 1, 1);
	
}
