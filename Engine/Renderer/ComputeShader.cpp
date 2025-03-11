#include "ComputeShader.h"

#include "VFS/ZVFS.h"
#include "RendererContext.h"

#include "Core/Logger.h"

#include "Asset/ShaderPermutationFile.h"

using namespace ENGINE_NAMESPACE;

extern std::vector<uint8_t> GetShaderBinaryData(SpfFile& file, std::string PermutationKey, Render::RendererAPI api);

Render::ComputeShader::ComputeShader(const std::string_view path, uint32_t permIndex)
{
    Ref<SpfFile> cShaderStream;

    cShaderStream = CreateRef<SpfFile>(path.data());

    std::vector<uint8_t> binary = GetShaderBinaryData(*cShaderStream, std::string("PERMUTATION_").append(std::to_string(permIndex)).append("\n"), RendererContext::get_api());

}

Render::ComputeShader::~ComputeShader()
{
}
