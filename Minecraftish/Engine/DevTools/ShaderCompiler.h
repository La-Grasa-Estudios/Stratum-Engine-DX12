#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "Asset/ShaderPermutationFile.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Core/JobManager.h"
#include "Renderer/RendererContext.h"
#include "Util/ShaderReflection.h"
#include "VFS/ZVFS.h"

#include "md5.h"
#include <bitset>

BEGIN_ENGINE

class GLSLToSpirV {

public:

	enum class shader_type {
		fragment,
		vertex,
		geometry,
		compute
	};

	inline static const char* g_Apis[3] = {
		"D3D",
		"D3D",
	};

	static std::vector<uint8_t> GetShaderBinary(RefBinaryStream& ss, ShaderPreprocessor& processor, const char* input, GLSLToSpirV::shader_type type, int permIndex, std::vector<std::string> defines, std::vector<shaderbinding_t>& shaderBindings, Render::RendererAPI targetApi);

	/// <summary>
	/// Vertex and Pixel(Fragment) shaders are compiled as a single file now, to compile either specify fragment or vertex
	/// </summary>
	static bool build_object(const char* input, const char* output, shader_type type, int nbPermutations = 1);

};

END_ENGINE