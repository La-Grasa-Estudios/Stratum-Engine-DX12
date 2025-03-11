#include "GraphicsPipeline.h"

#include "DX12/GlobalSharedData.h"

#include "VFS/ZVFS.h"
#include "RendererContext.h"

#include "Core/Logger.h"

#include "Asset/ShaderPermutationFile.h"
#include "json/json.hpp"

#include <bitset>
#include <d3dcompiler.h>

using namespace ENGINE_NAMESPACE;

void Render::PipelineDescription::RequirePermutation(const std::string& key)
{
    PermutationKey += key;
    PermutationKey += '\n';
}

Render::PipelineDescription::PipelineDescription()
{
    this->VertexShaderPath = "";
    this->PixelShaderPath = "";
    this->GeometryShaderPath = "";
    this->PermutationKey.clear();
}

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

std::vector<uint8_t> GetShaderBinaryData(SpfFile& file, std::string PermutationKey, Render::RendererAPI api) {

    const char* API_KEYS[] =
    {
        "ogl",
        "dx",
        "dx"
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
    std::string fileName = apiKey.append(bitset.to_string()).append(".shader");

    if (!file.Files.contains(fileName))
    {
        bitset = {};
        apiKey = API_KEYS[(int)api];
        fileName = apiKey.append(bitset.to_string()).append(".shader");
    }

    auto f = file.Files[fileName].get();

    std::vector<uint8_t> binary;
    binary.resize(f->ContentsLen);

    file.ReadFile(f, (char*)binary.data());

    return binary;

}

using namespace ENGINE_NAMESPACE::Render;

#define eturn return
#define gosleep return
#define its4am return
#define no return

DXGI_FORMAT EngineFormatToDXGI(VertexType type) {

    switch (type)
    {

    case ENGINE_NAMESPACE::Render::VertexType::INT16_NORM:
        return DXGI_FORMAT_R16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT2_16_NORM:
        return DXGI_FORMAT_R16G16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT4_16_NORM:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT16_NORM:
        return DXGI_FORMAT_R16_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT2_16_NORM:
        return DXGI_FORMAT_R16G16_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT4_16_NORM:
        return DXGI_FORMAT_R16G16B16A16_UNORM;

    case ENGINE_NAMESPACE::Render::VertexType::INT8_NORM:
        return DXGI_FORMAT_R8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT2_8_NORM:
        return DXGI_FORMAT_R8G8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::INT4_8_NORM:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT8_NORM:
        return DXGI_FORMAT_R8_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT2_8_NORM:
        return DXGI_FORMAT_R8G8_UNORM;
    case ENGINE_NAMESPACE::Render::VertexType::UINT4_8_NORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case ENGINE_NAMESPACE::Render::VertexType::UBYTE4:
        return DXGI_FORMAT_R8G8B8A8_UINT;

    case ENGINE_NAMESPACE::Render::VertexType::INT:
        return DXGI_FORMAT_R32_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::SHORT:
        return DXGI_FORMAT_R16_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::BYTE:
        return DXGI_FORMAT_R8_SINT;
    case ENGINE_NAMESPACE::Render::VertexType::UINT:
        return DXGI_FORMAT_R32_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::USHORT:
        return DXGI_FORMAT_R16_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::UBYTE:
        return DXGI_FORMAT_R8_UINT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT_32:
        return DXGI_FORMAT_R32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT2_32:
        return DXGI_FORMAT_R32G32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT3_32:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT4_32:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT_16:
        eturn DXGI_FORMAT_R16_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT2_16:
        its4am DXGI_FORMAT_R16G16_FLOAT;
    case ENGINE_NAMESPACE::Render::VertexType::FLOAT4_16:
        gosleep DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:
        no DXGI_FORMAT_R32G32B32_FLOAT;
    }

}

D3D12_INPUT_CLASSIFICATION EngineFormatToD3D12(VertexInputRate rate) {
    switch (rate)
    {
    case ENGINE_NAMESPACE::Render::VertexInputRate::PER_VERTEX:
        return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    case ENGINE_NAMESPACE::Render::VertexInputRate::PER_INSTANCE:
        return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
    default:
        return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }
}

std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12CreateInputLayout(const ShaderVertexLayout& VertexLayout) {

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputDescriptors;

    for (int i = 0; i < VertexLayout.VertexAttributes.size(); i++) {
        const VertexAttribute& attribute = VertexLayout.VertexAttributes[i];

        D3D12_INPUT_ELEMENT_DESC desc = { "TEXCOORD", attribute.Index, EngineFormatToDXGI(attribute.Type), 0, attribute.Offset, EngineFormatToD3D12(attribute.InputRate), 0 };
        inputDescriptors.push_back(desc);
    }
    return inputDescriptors;
}

std::vector<CD3DX12_ROOT_PARAMETER1> GetRootParameters(ID3D12ShaderReflection* pReflection, const std::vector<ShaderConstants>& constants, D3D12_SHADER_VISIBILITY flags)
{
    std::vector<CD3DX12_ROOT_PARAMETER1> RootParameters;
    uint32_t textureCount = 0;

    D3D12_SHADER_DESC shaderDesc;
    pReflection->GetDesc(&shaderDesc);


    for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; i++)
    {
        ID3D12ShaderReflectionConstantBuffer* cb = pReflection->GetConstantBufferByIndex(i);

        CD3DX12_ROOT_PARAMETER1 rootParameter{};
        rootParameter.InitAsConstantBufferView(i, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, flags);

        RootParameters.push_back(rootParameter);
    }

    for (uint32_t i = 0; i < constants.size(); i++)
    {
        CD3DX12_ROOT_PARAMETER1 rootParameter{};
        ShaderConstants constant = constants[i];

        if (constant.visibility == ShaderVisibility::VERTEX && flags != D3D12_SHADER_VISIBILITY_VERTEX)
        {
            continue;
        }

        if (constant.visibility == ShaderVisibility::PIXEL && flags != D3D12_SHADER_VISIBILITY_PIXEL)
        {
            continue;
        }
        
        if (constant.index > RootParameters.size())
        {
            throw std::out_of_range("Root constant index out of range!");
        }
        rootParameter.InitAsConstants(constant.size / sizeof(uint32_t), 0, constant.index, flags);
        RootParameters[constant.index] = rootParameter;
    }

    int32_t lastShaderRegister = -1;
    int32_t baseShaderRegister = -1;

    std::vector<CD3DX12_ROOT_PARAMETER1> samplers{};

    for (uint32_t i = 0; i < shaderDesc.BoundResources; i++)
    {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
        pReflection->GetResourceBindingDesc(i, &bindDesc);

        if (bindDesc.Type == D3D_SIT_TEXTURE)
        {
            if (baseShaderRegister == -1)
            {
                baseShaderRegister = bindDesc.BindPoint;
            }

            if (textureCount > 0 && bindDesc.BindPoint - 1 != lastShaderRegister)
            {
                CD3DX12_DESCRIPTOR_RANGE1* descriptorRange = new CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureCount, baseShaderRegister);
                CD3DX12_DESCRIPTOR_RANGE1* descriptorRangeS = new CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, textureCount, baseShaderRegister);

                CD3DX12_ROOT_PARAMETER1 rootParameter{};
                CD3DX12_ROOT_PARAMETER1 rootParameterS{};

                rootParameter.InitAsDescriptorTable(1, descriptorRange, flags);
                rootParameterS.InitAsDescriptorTable(1, descriptorRangeS, flags);

                baseShaderRegister = bindDesc.BindPoint;
                textureCount = 0;
                RootParameters.push_back(rootParameter);
                samplers.push_back(rootParameterS);
            }

            textureCount++;
            lastShaderRegister = bindDesc.BindPoint;

        }

    }

    if (textureCount > 0)
    {
        CD3DX12_DESCRIPTOR_RANGE1* descriptorRange = new CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureCount, baseShaderRegister);
        CD3DX12_DESCRIPTOR_RANGE1* descriptorRangeS = new CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, textureCount, baseShaderRegister);

        CD3DX12_ROOT_PARAMETER1 rootParameter{};
        CD3DX12_ROOT_PARAMETER1 rootParameterS{};

        rootParameter.InitAsDescriptorTable(1, descriptorRange, flags);
        rootParameterS.InitAsDescriptorTable(1, descriptorRangeS, flags);

        RootParameters.push_back(rootParameter);
        samplers.push_back(rootParameterS);
    }

    for (auto& p : samplers)
    {
        RootParameters.push_back(p);
    }

    return RootParameters;

}

std::vector<CD3DX12_ROOT_PARAMETER1> MergeConstantBuffers(const std::vector<CD3DX12_ROOT_PARAMETER1>& parameters, uint32_t* cbCount)
{
    std::unordered_map<uint32_t, CD3DX12_ROOT_PARAMETER1> cbMap;
    std::vector<CD3DX12_ROOT_PARAMETER1> newParameters;

    *cbCount = 0;

    for (auto& p : parameters)
    {
        if (p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV || p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            if (!cbMap.contains(p.Descriptor.ShaderRegister))
            {
                cbMap[p.Descriptor.ShaderRegister] = p;
                *cbCount += 1;
            }
            else
            {
                continue;
            }
        }
        newParameters.push_back(p);
    }

    return newParameters;
}

Render::GraphicsPipeline::GraphicsPipeline(PipelineDescription desc)
{
    this->NativePointer = new DX12::DX12PsoNativeData();

    this->ShaderDesc = desc;

    Ref<SpfFile> vShaderStream, fShaderStream, gShaderStream;

    vShaderStream = CreateRef<SpfFile>(ShaderDesc.VertexShaderPath);
    fShaderStream = CreateRef<SpfFile>(ShaderDesc.PixelShaderPath);

    if (!ShaderDesc.GeometryShaderPath.empty()) {
        gShaderStream = CreateRef<SpfFile>(ShaderDesc.GeometryShaderPath);
    }

    auto SourceVertex = GetShaderBinaryData(*vShaderStream, ShaderDesc.PermutationKey, RendererContext::get_api());
    auto SourcePixel = GetShaderBinaryData(*fShaderStream, ShaderDesc.PermutationKey, RendererContext::get_api());

    ID3DBlob* vs_blob_ptr = NULL, * ps_blob_ptr = NULL;

    D3DCreateBlob(SourceVertex.size(), &vs_blob_ptr);
    D3DCreateBlob(SourcePixel.size(), &ps_blob_ptr);

    memcpy(vs_blob_ptr->GetBufferPointer(), SourceVertex.data(), SourceVertex.size());
    memcpy(ps_blob_ptr->GetBufferPointer(), SourcePixel.data(), SourcePixel.size());


    ID3D12ShaderReflection* vsReflect;
    ID3D12ShaderReflection* psReflect;
    D3DReflect(vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), IID_PPV_ARGS(&vsReflect));
    D3DReflect(ps_blob_ptr->GetBufferPointer(), ps_blob_ptr->GetBufferSize(), IID_PPV_ARGS(&psReflect));

    auto vsRootParameters = GetRootParameters(vsReflect, ShaderDesc.Constants, D3D12_SHADER_VISIBILITY_VERTEX);
    auto psRootParameters = GetRootParameters(psReflect, ShaderDesc.Constants, D3D12_SHADER_VISIBILITY_PIXEL);

    this->As<DX12::DX12PsoNativeData>()->VsRootParameters = vsRootParameters;
    this->As<DX12::DX12PsoNativeData>()->PsRootParameters = psRootParameters;

    std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;

    for (auto& p : vsRootParameters)
    {
        rootParameters.push_back(p);
    }

    for (auto& p : psRootParameters)
    {
        rootParameters.push_back(p);
    }

    uint32_t cbCount;
    rootParameters = MergeConstantBuffers(rootParameters, &cbCount);

    As<DX12::DX12PsoNativeData>()->CBIndexes.resize(cbCount);

    for (uint32_t i = 0; i < rootParameters.size(); i++)
    {
        auto& p = rootParameters[i];
        if (p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV || p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            As<DX12::DX12PsoNativeData>()->CBIndexes[p.Descriptor.ShaderRegister] = i;
        }
    }

    auto InputLayout = D3D12CreateInputLayout(desc.VertexLayout);

    auto device = dxSharedData->Device.Get();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(rootParameters.size(), rootParameters.data(), 0, nullptr, rootSignatureFlags);

    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob);

    if (errorBlob) {
        printf((char*)errorBlob->GetBufferPointer());
        errorBlob->Release();
    }

    // Create the root signature.
    device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

    As<DX12::DX12PsoNativeData>()->RootSignature = rootSignature;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.InputLayout = { InputLayout.data(), (uint32_t)InputLayout.size() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs_blob_ptr);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps_blob_ptr);
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    //psoDesc.DSVFormat = DX12::g_ImageFormatTable[(int)desc.DepthTargetFormat];

    psoDesc.NumRenderTargets = desc.NumRenderTargets;

    psoDesc.RasterizerState.CullMode = (D3D12_CULL_MODE)desc.RasterizerState.CullMode;
    psoDesc.RasterizerState.FillMode = (D3D12_FILL_MODE)desc.RasterizerState.FillMode;
    psoDesc.RasterizerState.FrontCounterClockwise = false;
    psoDesc.RasterizerState.DepthClipEnable = desc.RasterizerState.DepthTest;

    psoDesc.DepthStencilState.DepthEnable = desc.StencilState.DepthEnable;
    psoDesc.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC)desc.StencilState.DepthFunc;
    psoDesc.DepthStencilState.DepthWriteMask = desc.StencilState.DepthEnableWriting ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = desc.StencilState.StencilEnable;
    psoDesc.DepthStencilState.StencilReadMask = desc.StencilState.StencilReadMask;
    psoDesc.DepthStencilState.StencilWriteMask = desc.StencilState.StencilWriteMask;

    psoDesc.DepthStencilState.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC)desc.StencilState.BackFace.StencilFunc;
    psoDesc.DepthStencilState.BackFace.StencilFailOp = (D3D12_STENCIL_OP)desc.StencilState.BackFace.StencilFailOp;
    psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = (D3D12_STENCIL_OP)desc.StencilState.BackFace.StencilDepthFailOp;
    psoDesc.DepthStencilState.BackFace.StencilPassOp = (D3D12_STENCIL_OP)desc.StencilState.BackFace.StencilPassOp;

    psoDesc.DepthStencilState.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)desc.StencilState.FrontFace.StencilFunc;
    psoDesc.DepthStencilState.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)desc.StencilState.FrontFace.StencilFailOp;
    psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)desc.StencilState.FrontFace.StencilDepthFailOp;
    psoDesc.DepthStencilState.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)desc.StencilState.FrontFace.StencilPassOp;

    psoDesc.BlendState.AlphaToCoverageEnable = false;
    psoDesc.BlendState.IndependentBlendEnable = desc.BlendState.IndependentBlendEnable;

    for (int i = 0; i < 8; i++) {
        D3D12_RENDER_TARGET_BLEND_DESC rtbd;
        ZeroMemory(&rtbd, sizeof(rtbd));

        rtbd.BlendEnable = desc.BlendState.BlendStates[i].EnableBlend;
        rtbd.SrcBlend = (D3D12_BLEND)desc.BlendState.BlendStates[i].SrcBlend;
        rtbd.DestBlend = (D3D12_BLEND)desc.BlendState.BlendStates[i].DestBlend;
        rtbd.SrcBlendAlpha = (D3D12_BLEND)desc.BlendState.BlendStates[i].SrcBlendAlpha;
        rtbd.DestBlendAlpha = (D3D12_BLEND)desc.BlendState.BlendStates[i].DestBlendAlpha;
        rtbd.BlendOp = (D3D12_BLEND_OP)desc.BlendState.BlendStates[i].BlendOperator;
        rtbd.BlendOpAlpha = (D3D12_BLEND_OP)desc.BlendState.BlendStates[i].BlendOperatorAlpha;
        rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


        psoDesc.BlendState.RenderTarget[i] = rtbd;
    }

    psoDesc.SampleMask = 0xFFFFFFFF;
    psoDesc.SampleDesc.Count = 1;

    for (int i = 0; i < desc.NumRenderTargets; i++)
    {
        psoDesc.RTVFormats[i] = DX12::g_ImageFormatTable[(int)desc.RenderTargetFormats[i]];
    }

    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&this->As<DX12::DX12PsoNativeData>()->PipelineState));

    vs_blob_ptr->Release();
    ps_blob_ptr->Release();

    for (auto& p : rootParameters)
    {
        if (p.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            delete p.DescriptorTable.pDescriptorRanges;
        }
    }

}

Render::GraphicsPipeline::~GraphicsPipeline()
{
    delete As<DX12::DX12PsoNativeData>();
}

Render::ShaderConstants::ShaderConstants(size_t Size, uint32_t Index, ShaderVisibility Vis)
{
    if (Size % 4 != 0 || Size < 4)
    {
        throw std::invalid_argument("Root constant size must be a multiple of 4!");
    }
    this->size = Size;
    this->index = Index;
    this->visibility = Vis;
}
