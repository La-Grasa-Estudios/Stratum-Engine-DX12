#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "Renderer/RendererContext.h"
#include "VFS/ZVFS.h"
#include "Core/Logger.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "Core/Time.h"
#include "Core/JobManager.h"

#include "Asset/ShaderPermutationFile.h"

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

	DLLEXPORT static std::vector<uint32_t> GetShaderBinary(RefBinaryStream& ss, ShaderPreprocessor& processor, const char* input, GLSLToSpirV::shader_type type, int permIndex, std::vector<std::string> defines, Render::RendererAPI targetApi);
	
	static bool build_object(const char* input, const char* output, shader_type type, int nbPermutations = 1);

};

END_ENGINE