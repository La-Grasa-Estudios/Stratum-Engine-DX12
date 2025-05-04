#include "GraphicsPipeline.h"

#include "VFS/ZVFS.h"
#include "RendererContext.h"

#include "Core/Logger.h"

#include "Asset/ShaderPermutationFile.h"
#include "Util/ShaderReflection.h"

#include "json/json.hpp"

#include <bitset>

using namespace ENGINE_NAMESPACE;

void Render::PipelineDescription::RequirePermutation(const std::string& key)
{
    PermutationKey += key;
    PermutationKey += '\n';
}

void Render::PipelineDescription::SetStaticBinding(const nvrhi::static_vector<nvrhi::BindingSetItem, 64>& set)
{
    UseStaticBinding = true;
    StaticBindingItems = set;
}

void Render::PipelineDescription::AddBindlessItem(const nvrhi::BindingLayoutItem& item)
{
    BindlessItems.push_back(item);
    UseBindless = true;
}

Render::PipelineDescription::PipelineDescription()
{
    this->ShaderPath = "";
    this->PermutationKey.clear();
}

/*
void Render::PipelineDescription::FromFile(std::string_view path)
{
    this->PermutationKey.clear();
    std::string file = "sdef/";
    file.append(path.data());
    RefBinaryStream binaryStream = ZVFS::GetFile(file.c_str());

    if (binaryStream->Str().empty()) {
        Z_WARN("Failed to load PipelineDescription file {}", path.data());
        return;
    }

    std::string str = binaryStream->Str();

    try
    {
        nlohmann::json js = nlohmann::json::parse(str);

        auto vertexShader = js["VS"];
        auto pixelShader = js["PS"];

        this->PixelShaderPath = std::string("shaders/").append(pixelShader);
        this->VertexShaderPath = std::string("shaders/").append(vertexShader);
        this->GeometryShaderPath = "";
    }
    catch (const std::exception& e)
    {
        ErrorLog(e.what());
    }

}
*/

std::vector<uint8_t> GetShaderBinaryData(SpfFile& file, std::string kind, std::string PermutationKey, Render::RendererAPI api) {

    const char* API_KEYS[] =
    {
        "dx11",
        "dx12",
        "vk"
    };

    std::map<std::string, uint32_t> permutationTable{};

    for (auto& kp : file.Metadata)
    {
        permutationTable[kp.first] = std::stoi(kp.second);
    }

    std::bitset<64> bitset{};

    std::string str;
    for (int i = 0; i < PermutationKey.size(); i++)
    {
        if (PermutationKey[i] == '\n')
        {
            if (!str.empty())
            {
                if (permutationTable.contains(str))
                    bitset.set(permutationTable[str]);
            }
            str.clear();
            continue;
        }
        str += PermutationKey[i];
    }

    std::string apiKey = API_KEYS[(int)api];
    std::string fileName = apiKey.append(kind).append(bitset.to_string()).append(".shader");

    if (!file.Files.contains(fileName))
    {
        bitset = {};
        apiKey = "dx11";
        fileName = apiKey.append(kind).append(bitset.to_string()).append(".shader");
        auto f = file.Files[fileName].get();

        if (!f)
        {
            apiKey = "dx12";
            fileName = apiKey.append(kind).append(bitset.to_string()).append(".shader");
            f = file.Files[fileName].get();
        }

        if (!f)
        {
            apiKey = "dx12";
            fileName = apiKey.append(bitset.to_string()).append(".shader");
            f = file.Files[fileName].get();
        }

    }

    auto f = file.Files[fileName].get();

    std::vector<uint8_t> binary;
    binary.resize(f->ContentsLen);

    file.ReadFile(f, (char*)binary.data());

    return binary;

}

std::vector<shaderbinding_t> GetShaderBindings(SpfFile& file, std::string kind, std::string PermutationKey, Render::RendererAPI api)
{
    const char* API_KEYS[] =
    {
        "dx11",
        "dx12",
        "vk"
    };

    std::map<std::string, uint32_t> permutationTable{};

    for (auto& kp : file.Metadata)
    {
        permutationTable[kp.first] = std::stoi(kp.second);
    }

    std::bitset<64> bitset{};

    std::string str;
    for (int i = 0; i < PermutationKey.size(); i++)
    {
        if (PermutationKey[i] == '\n')
        {
            if (!str.empty())
            {
                if (permutationTable.contains(str))
                    bitset.set(permutationTable[str]);
            }
            str.clear();
            continue;
        }
        str += PermutationKey[i];
    }

    std::string apiKey = API_KEYS[(int)api];
    std::string fileName = apiKey.append(kind).append(bitset.to_string()).append(".sig");

    if (!file.Files.contains(fileName))
    {
        bitset = {};
        apiKey = API_KEYS[(int)api];
        fileName = apiKey.append(kind).append(bitset.to_string()).append(".sig");
    }

    if (!file.Files.contains(fileName))
    {
        return {};
    }

    auto f = file.Files[fileName].get();

    SpfFileStream stream(file, f);

    uint32_t numBindings = 0;
    stream.read(&numBindings, sizeof(uint32_t));

    std::vector<shaderbinding_t> bindings;

    for (int i = 0; i < numBindings; i++)
    {
        shaderbinding_t binding{};
        stream.read(&binding.Type, sizeof(uint8_t));
        stream.read(&binding.Dimension, sizeof(uint8_t));
        stream.read(&binding.BindingPoint, sizeof(uint8_t));
        stream.read(&binding.BindingSpace, sizeof(uint8_t));
        bindings.push_back(binding);
    }

    return bindings;

}

using namespace ENGINE_NAMESPACE::Render;

#define eturn return
#define gosleep return
#define its4am return
#define no return

nvrhi::Format ConvertEngineFormatToNV(VertexType type) {

    switch (type)
    {

    case ENGINE_NAMESPACE::Render::VertexType::INT16_NORM:
        return nvrhi::Format::R16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT2_16_NORM:
        return nvrhi::Format::RG16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT4_16_NORM:
        return nvrhi::Format::RGBA16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT16_NORM:
        return nvrhi::Format::R16_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT2_16_NORM:
        return nvrhi::Format::RG16_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT4_16_NORM:
        return nvrhi::Format::RGBA16_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT8_NORM:
        return nvrhi::Format::R8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT2_8_NORM:
        return nvrhi::Format::RG8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT4_8_NORM:
        return nvrhi::Format::RGBA8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT8_NORM:
        return nvrhi::Format::R8_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT2_8_NORM:
        return nvrhi::Format::RG8_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT4_8_NORM:
        return nvrhi::Format::RGBA8_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UBYTE4:
        return nvrhi::Format::RGBA8_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::INT:
        return nvrhi::Format::R32_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::SHORT:
        return nvrhi::Format::R16_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::BYTE:
        return nvrhi::Format::R8_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::UINT:
        return nvrhi::Format::R32_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::USHORT:
        return nvrhi::Format::R16_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::UBYTE:
        return nvrhi::Format::R8_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT_32:
        return nvrhi::Format::R32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT2_32:
        return nvrhi::Format::RG32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT3_32:
        return nvrhi::Format::RGB32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT4_32:
        return nvrhi::Format::RGBA32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT_16:
        eturn nvrhi::Format::R16_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT2_16:
        its4am nvrhi::Format::RG16_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT4_16:
        gosleep nvrhi::Format::RGBA16_FLOAT;
    default:
        no nvrhi::Format::RGB32_FLOAT;
    }

}

static std::vector<nvrhi::VertexAttributeDesc> GetVertexAttributeDesc(const ShaderVertexLayout& layout)
{
    std::vector<nvrhi::VertexAttributeDesc> inputDescriptors;

    for (int i = 0; i < layout.VertexAttributes.size(); i++) {
        const VertexAttribute& attribute = layout.VertexAttributes[i];

        auto att = nvrhi::VertexAttributeDesc();
        
        att.setName("TEXCOORD");
        att.setIndex(i);
        att.setIsInstanced(attribute.InputRate == VertexInputRate::PER_INSTANCE);
        att.setElementStride(layout.Stride);
        att.setFormat(ConvertEngineFormatToNV(attribute.Type));
        att.setOffset(attribute.Offset);
        att.setBufferIndex(attribute.BufferIndex);

        inputDescriptors.push_back(att);

    }

    return inputDescriptors;
}

Render::GraphicsPipeline::GraphicsPipeline(PipelineDescription desc)
{

    this->ShaderDesc = desc;
    
    if (desc.RenderTarget && desc.RenderTarget->IsWindowFramebuffer())
        RendererContext::s_Context->RegisterBackBufferPipeline(this);

    Ref<SpfFile> ShaderStream;

    ShaderStream = CreateRef<SpfFile>(ShaderDesc.ShaderPath);

    std::vector<shaderbinding_t> bindings;

    {
        auto bvs = GetShaderBindings(*ShaderStream, "vertex", ShaderDesc.PermutationKey, RendererContext::get_api());
        auto bps = GetShaderBindings(*ShaderStream, "pixel", ShaderDesc.PermutationKey, RendererContext::get_api());

        for (int i = 0; i < bvs.size(); i++)
        {
            bindings.push_back(bvs[i]);
        }

        for (int i = 0; i < bps.size(); i++)
        {
            bool isFree = true;
            for (int j = 0; j < bvs.size(); j++)
            {
                if (
                    bps[i].Type == bvs[j].Type &&
                    bps[i].Dimension == bvs[j].Dimension &&
                    bps[i].BindingPoint == bvs[j].BindingPoint
                    )
                {
                    isFree = false;
                }
            }

            if (isFree)
                bindings.push_back(bps[i]);
        }
    }

    auto bindlessLayoutDesc = nvrhi::BindlessLayoutDesc()
        .setMaxCapacity(desc.BindlessCapacity)
        .setVisibility(nvrhi::ShaderType::All);

    uint32_t firstSlot = 0;

    for (int i = 0; i < bindings.size(); i++)
    {
        auto& binding = bindings[i];

        auto layout = &bindlessLayoutDesc;

        if (binding.BindingSpace == 0) // Check for bindless space ex: register(t0, space1)
        {
            continue;
        }

        binding.BindingPoint = binding.BindingSpace;

        if (binding.Type == ShaderReflectionResourceType::TEXTURE)
        {
            bindlessLayoutDesc.addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(binding.BindingPoint));
        }
        if (binding.Type == ShaderReflectionResourceType::CBUFFER)
        {
            bindlessLayoutDesc.addRegisterSpace(nvrhi::BindingLayoutItem::ConstantBuffer(binding.BindingPoint));
        }
        if (binding.Type == ShaderReflectionResourceType::SAMPLER)
        {
            bindlessLayoutDesc.addRegisterSpace(nvrhi::BindingLayoutItem::Sampler(binding.BindingPoint));
        }
        if (binding.Type == ShaderReflectionResourceType::STRUCTURED_BUFFER)
        {
            bindlessLayoutDesc.addRegisterSpace(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(binding.BindingPoint));
        }
        if (binding.Type == ShaderReflectionResourceType::BYTEADDRESS)
        {
            bindlessLayoutDesc.addRegisterSpace(nvrhi::BindingLayoutItem::RawBuffer_SRV(binding.BindingPoint));
        }
    }

    bindlessLayoutDesc.setFirstSlot(0);

    bool enableBindless = ShaderDesc.UseBindless || bindlessLayoutDesc.registerSpaces.size() > 0;

    if (enableBindless)
    {
        this->BindlessLayout = RendererContext::GetDevice()->createBindlessLayout(bindlessLayoutDesc);
    }
}

Render::GraphicsPipeline::~GraphicsPipeline()
{
    
}

void Render::GraphicsPipeline::UpdatePipeline()
{
    PipelineDescription desc = this->ShaderDesc;
    Ref<SpfFile> ShaderStream;

    ShaderStream = CreateRef<SpfFile>(ShaderDesc.ShaderPath);

    auto SourceVertex = GetShaderBinaryData(*ShaderStream, "vertex", ShaderDesc.PermutationKey, RendererContext::get_api());
    auto SourcePixel = GetShaderBinaryData(*ShaderStream, "pixel", ShaderDesc.PermutationKey, RendererContext::get_api());
    
    std::vector<shaderbinding_t> bindings;

    {
        auto bvs = GetShaderBindings(*ShaderStream, "vertex", ShaderDesc.PermutationKey, RendererContext::get_api());
        auto bps = GetShaderBindings(*ShaderStream, "pixel", ShaderDesc.PermutationKey, RendererContext::get_api());

        for (int i = 0; i < bvs.size(); i++)
        {
            bindings.push_back(bvs[i]);
        }

        for (int i = 0; i < bps.size(); i++)
        {
            bool isFree = true;
            for (int j = 0; j < bvs.size(); j++)
            {
                if (
                    bps[i].Type == bvs[j].Type &&
                    bps[i].Dimension == bvs[j].Dimension &&
                    bps[i].BindingPoint == bvs[j].BindingPoint
                    )
                {
                    isFree = false;
                }
            }

            if (isFree)
                bindings.push_back(bps[i]);
        }
    }

    auto vertexShader = RendererContext::GetDevice()->createShader(
        nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex),
        SourceVertex.data(), SourceVertex.size());

    auto pixelShader = RendererContext::GetDevice()->createShader(
        nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel),
        SourcePixel.data(), SourcePixel.size());

    auto InputLayout = GetVertexAttributeDesc(desc.VertexLayout);

    nvrhi::InputLayoutHandle inputLayout = RendererContext::GetDevice()->createInputLayout(
        InputLayout.data(), InputLayout.size(), vertexShader);

    auto layoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All);    

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

            if (binding.BindingSpace >= 1) // Bindless Space
            {
                continue;
            }

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

    bool enableBindless = ShaderDesc.UseBindless || this->BindlessLayout;

    this->ShaderDesc.BindingItems.clear();

    for (int i = 0; i < layoutDesc.bindings.size(); i++)
    {
        this->ShaderDesc.BindingItems.push_back(layoutDesc.bindings[i]);
    }

    nvrhi::BindingLayoutHandle bindingLayout = RendererContext::GetDevice()->createBindingLayout(layoutDesc);

    nvrhi::RenderState renderState;

    renderState.rasterState.cullMode = (nvrhi::RasterCullMode)desc.RasterizerState.CullMode;
    renderState.rasterState.fillMode = (nvrhi::RasterFillMode)desc.RasterizerState.FillMode;
    renderState.rasterState.frontCounterClockwise = true;
    renderState.rasterState.depthClipEnable = desc.RasterizerState.DepthTest;

    renderState.depthStencilState.depthTestEnable = desc.StencilState.DepthEnable;
    renderState.depthStencilState.depthFunc = (nvrhi::ComparisonFunc)desc.StencilState.DepthFunc;
    renderState.depthStencilState.depthWriteEnable = desc.StencilState.DepthEnableWriting;
    renderState.depthStencilState.stencilEnable = desc.StencilState.StencilEnable;
    renderState.depthStencilState.stencilReadMask = desc.StencilState.StencilReadMask;
    renderState.depthStencilState.stencilWriteMask = desc.StencilState.StencilWriteMask;
    renderState.depthStencilState.setStencilRefValue(desc.StencilState.StencilRef);;

    renderState.depthStencilState.backFaceStencil.stencilFunc = (nvrhi::ComparisonFunc)desc.StencilState.BackFace.StencilFunc;
    renderState.depthStencilState.backFaceStencil.failOp = (nvrhi::StencilOp)desc.StencilState.BackFace.StencilFailOp;
    renderState.depthStencilState.backFaceStencil.depthFailOp = (nvrhi::StencilOp)desc.StencilState.BackFace.StencilDepthFailOp;
    renderState.depthStencilState.backFaceStencil.passOp = (nvrhi::StencilOp)desc.StencilState.BackFace.StencilPassOp;

    renderState.depthStencilState.frontFaceStencil.stencilFunc = (nvrhi::ComparisonFunc)desc.StencilState.FrontFace.StencilFunc;
    renderState.depthStencilState.frontFaceStencil.failOp = (nvrhi::StencilOp)desc.StencilState.FrontFace.StencilFailOp;
    renderState.depthStencilState.frontFaceStencil.depthFailOp = (nvrhi::StencilOp)desc.StencilState.FrontFace.StencilDepthFailOp;
    renderState.depthStencilState.frontFaceStencil.passOp = (nvrhi::StencilOp)desc.StencilState.FrontFace.StencilPassOp;

    renderState.blendState.alphaToCoverageEnable = false;

    auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
        .setInputLayout(inputLayout)
        .setVertexShader(vertexShader)
        .setPixelShader(pixelShader)
        .setRenderState(renderState);

    pipelineDesc.addBindingLayout(bindingLayout);

    if (enableBindless)
    {
        pipelineDesc.bindingLayouts = { bindingLayout, BindlessLayout };
    }

    nvrhi::IFramebuffer* fbHandle;

    if (desc.RenderTarget->IsWindowFramebuffer())
    {
        fbHandle = RendererContext::s_Context->NvFramebufferRtvs[0];
    }
    else
    {
        fbHandle = desc.RenderTarget->Handle;
    }

    PipelineHandle = RendererContext::GetDevice()->createGraphicsPipeline(pipelineDesc, fbHandle);
    BindingLayout = bindingLayout;
}

void Render::GraphicsPipeline::SetRenderTarget(Ref<Framebuffer> rt)
{
    ShaderDesc.RenderTarget = rt.get();
    PipelineHandle = nullptr;
}

void Render::GraphicsPipeline::UpdateStaticBinding(const StaticBindingTable& setDesc)
{
    ShaderDesc.SetStaticBinding(setDesc);
    StaticBindingSet = nullptr;
}
