#include "ComputePipeline.h"

#include "VFS/ZVFS.h"
#include "RendererContext.h"

#include "Core/Logger.h"

#include "Util/ShaderReflection.h"

#include "Asset/ShaderPermutationFile.h" 

using namespace ENGINE_NAMESPACE;

extern std::vector<uint8_t> GetShaderBinaryData(SpfFile& file, std::string kind, std::string PermutationKey, Render::RendererAPI api);
extern std::vector<shaderbinding_t> GetShaderBindings(SpfFile& file, std::string kind, std::string PermutationKey, Render::RendererAPI api);

Render::ComputePipeline::ComputePipeline(const ComputePipelineDesc& desc)
{
    Ref<SpfFile> cShaderStream;

    cShaderStream = CreateRef<SpfFile>(desc.path.data());

    std::vector<uint8_t> binary = GetShaderBinaryData(*cShaderStream, "comp", std::string("PERMUTATION_").append(std::to_string(desc.permIndex)).append("\n"), RendererContext::get_api());
    auto bindings = GetShaderBindings(*cShaderStream, "comp", std::string("PERMUTATION_").append(std::to_string(desc.permIndex)).append("\n"), RendererContext::get_api());

    auto shader = RendererContext::GetDevice()->createShader(
        nvrhi::ShaderDesc(nvrhi::ShaderType::Compute),
        binary.data(), binary.size());    

    auto layoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Compute);

    if (bindings.empty())
    {
        for (int i = 0; i < desc.BindingItems.size(); i++)
        {
            layoutDesc.addItem(desc.BindingItems[i]);
        }
    }
    else
    {
        for (int i = 0; i < bindings.size(); i++)
        {
            auto& binding = bindings[i];

            if (binding.Type == ShaderReflectionResourceType::TEXTURE)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(binding.BindingPoint));
            }
            if (binding.Type == ShaderReflectionResourceType::CBUFFER)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(binding.BindingPoint));
            }
            if (binding.Type == ShaderReflectionResourceType::SAMPLER)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(binding.BindingPoint));
            }
            if (binding.Type == ShaderReflectionResourceType::UAV_RW_BYTEADDRESS)
            {
                if (binding.Dimension >= ShaderReflectionResourceDimension::TEXTURE1D && binding.Dimension <= ShaderReflectionResourceDimension::TEXTURECUBEARRAY)
                {
                    layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(binding.BindingPoint));
                }
                else
                {
                    layoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(binding.BindingPoint));
                }
            }
            if (binding.Type == ShaderReflectionResourceType::UAV_RW_TYPED)
            {
                if (binding.Dimension >= ShaderReflectionResourceDimension::TEXTURE1D && binding.Dimension <= ShaderReflectionResourceDimension::TEXTURECUBEARRAY)
                {
                    layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(binding.BindingPoint));
                }
                else
                {
                    layoutDesc.addItem(nvrhi::BindingLayoutItem::TypedBuffer_UAV(binding.BindingPoint));
                }
            }
            if (binding.Type == ShaderReflectionResourceType::UAV_RW_STRUCTURED)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_UAV(binding.BindingPoint));
            }
            if (binding.Type == ShaderReflectionResourceType::STRUCTURED_BUFFER)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(binding.BindingPoint));
            }
            if (binding.Type == ShaderReflectionResourceType::BYTEADDRESS)
            {
                layoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(binding.BindingPoint));
            }
        }
        for (int i = 0; i < desc.BindingItems.size(); i++)
        {
            if (desc.BindingItems[i].type == nvrhi::ResourceType::PushConstants)
            {
                for (int j = 0; j < layoutDesc.bindings.size(); j++)
                {
                    if (layoutDesc.bindings[j].type == nvrhi::ResourceType::ConstantBuffer && layoutDesc.bindings[j].slot == desc.BindingItems[i].slot)
                    {
                        layoutDesc.bindings[j] = desc.BindingItems[i];
                    }
                }
            }
        }
    }

    ShaderDesc = desc;
    this->ShaderDesc.BindingItems.clear();

    for (int i = 0; i < layoutDesc.bindings.size(); i++)
    {
        this->ShaderDesc.BindingItems.push_back(layoutDesc.bindings[i]);
    }

    nvrhi::BindingLayoutHandle bindingLayout = RendererContext::GetDevice()->createBindingLayout(layoutDesc);

    auto computeDesc = nvrhi::ComputePipelineDesc()
        .setComputeShader(shader)
        .addBindingLayout(bindingLayout);

    Handle = RendererContext::GetDevice()->createComputePipeline(computeDesc);
    BindingLayout = bindingLayout;

}

Render::ComputePipeline::~ComputePipeline()
{
}
