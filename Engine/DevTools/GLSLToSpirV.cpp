#include "GLSLToSpirV.h"

#ifdef NDEBUG
#pragma comment(lib, "shaderc_combined.lib")
#endif

#include "spirv_cross/spirv_hlsl.hpp"

#include "shaderc/shaderc.h"
#include "shaderc/shaderc.hpp"

#include <d3dcompiler.h>

using namespace ENGINE_NAMESPACE;

std::vector<uint32_t> GLSLToSpirV::GetShaderBinary(RefBinaryStream& ss, ShaderPreprocessor& processor, const char* input, GLSLToSpirV::shader_type type, int permIndex, std::vector<std::string> defines, Render::RendererAPI targetApi)
{
	shaderc::CompileOptions compileOptions;

	bool hlsl = false;

	compileOptions.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
	compileOptions.SetTargetSpirv(shaderc_spirv_version_1_0);
	compileOptions.SetSourceLanguage(shaderc_source_language_glsl);
	compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
	compileOptions.SetGenerateDebugInfo();

	std::string inputStr = input;
	if (inputStr.ends_with("hlsl")) {
		compileOptions.SetSourceLanguage(shaderc_source_language_hlsl);

		hlsl = true;
	}

	std::string stage = "#define STAGE_";

	shaderc::Compiler compiler;

	shaderc_shader_kind kind = shaderc_glsl_default_fragment_shader;

	switch (type)
	{
	case GLSLToSpirV::shader_type::fragment:
		kind = shaderc_glsl_fragment_shader;
		stage.append("PIXEL");
		break;
	case GLSLToSpirV::shader_type::vertex:
		kind = shaderc_glsl_vertex_shader;
		stage.append("VERTEX");
		break;
	case GLSLToSpirV::shader_type::geometry:
		kind = shaderc_glsl_geometry_shader;
		stage.append("GEOMETRY");
		break;
	case GLSLToSpirV::shader_type::compute:
		kind = shaderc_glsl_compute_shader;
		stage.append("COMPUTE");
		break;
	default:
		break;
	}

	std::string code = "";

	if (!hlsl) {
		code.append("#version 450 core\n");
	}
	else {

	}

	for (auto str : defines)
	{
		code.append("#define ");
		code.append(str);
		code.append("\n");
	}

	code.append("#define ");
	code.append(g_Apis[(int)targetApi]);
	code.append("\n");

	code.append("#define PERM_");
	code.append(std::to_string(permIndex));
	code.append("\n");

	code.append(stage);
	code.append("\n");

	std::string rawCode = ss->Str();

	code.append(processor.PreProcessSource(rawCode));

	auto binary = compiler.CompileGlslToSpv(code, kind, input, compileOptions);

	if (binary.GetCompilationStatus() != shaderc_compilation_status_success) {
		ErrorLog("Failed to compile" << input);
		ErrorLog(binary.GetErrorMessage().c_str());
		ErrorLog(code);
		std::vector<uint32_t> bin;
		return bin;
	}
	else {
		InfoLog(binary.GetErrorMessage().c_str());
	}

	std::vector<uint32_t> bin = { binary.cbegin(), binary.cend() };
	return bin;
}

bool GLSLToSpirV::build_object(const char* input, const char* output, shader_type type, int nbPermutations)
{
	RefBinaryStream ss = ENGINE_NAMESPACE::ZVFS::GetFile(input);

	if (ss->Str().empty()) return false;

	ShaderPreprocessor sourcePreprocessor;

	sourcePreprocessor.AddSearchPath("Engine/Include/");

	std::filesystem::path p1(input);
	p1.remove_filename();
	std::string p1s = p1.string();
	sourcePreprocessor.AddSearchPath(p1s);

	bool needsRecompile = false;

	std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
	auto since_epoch = begin.time_since_epoch();
	long start = (long)(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count());
	//Z_INFO("================ Shader Compilation Started ================");

	std::filesystem::file_time_type lastWrite = std::filesystem::last_write_time(input);
	if (std::filesystem::exists(output)) {
		std::filesystem::file_time_type soTime = std::filesystem::last_write_time(output);
		std::ifstream in(output, std::ios::binary);
		char header[6];

		in.read(header, 6);

		if (strncmp(header, "SPFPKG", 6) != 0) {
			needsRecompile = true;
		}
		if (!needsRecompile) {
			if (lastWrite > soTime) {
				needsRecompile = true;
				Z_INFO("Recompiling {}", input);
			}
			else {
				//Z_INFO("Analyzing Dependencies for {}", input);
				std::string code = ss->Str();
				std::filesystem::file_time_type current = soTime;
				std::filesystem::file_time_type newTime = current;
				sourcePreprocessor.AnalyzeDependencies(code, &newTime);
				if (newTime > current) {
					needsRecompile = true;
					Z_INFO("Recompiling {}", input);
				}
				else {
					needsRecompile = false;
					//Z_INFO("================ Shader Compilation: 0 successful, 0 errors, 2 updated ================");
					begin = std::chrono::system_clock::now();
					since_epoch = begin.time_since_epoch();
					long end = (long)(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count()) - start;
					//Z_INFO("================ Shader Compilation: took {} seconds ================", (float)(end / 1000.0f));
				}
			}
		}
	}
	else {
		needsRecompile = true;
	}

	if (!needsRecompile) return true;

	Z_INFO("Compiling {}", input);

	std::string code = ss->Str();
	std::filesystem::file_time_type newTime = {};
	sourcePreprocessor.AnalyzeDependencies(code, &newTime);

	const uint32_t permSize = sizeof(uint32_t) * 6;
	const uint32_t headerSize = 5 + sizeof(uint16_t) + permSize * nbPermutations;

	std::ofstream out(output, std::ios::binary);
	std::stringstream permData;

	uint32_t totalSize = 0;

	SpfHeader spfHeader{};
	SpfMetadataHeader spfMetaHeader{};
	spfMetaHeader.MetadataCount = 0;

	SpfFileIndex fileIndex{};
	fileIndex.FileCount = sourcePreprocessor.GetPermutationCount() * 2;

	std::map<std::string, SpfMetadataValueKeyPair*> permutationTable;
	std::map<std::string, uint32_t> permutationToId;
	std::vector<SpfFileMetadata*> fileMetadata;
	std::vector<uint8_t*> shaderData;
	std::binary_semaphore access(1);

	size_t MetaSize = 0;

	{
		uint32_t permIndex = 0;
		for (uint32_t i = 0; i < sourcePreprocessor.GetPermutationCount(); i++)
		{
			auto perm = sourcePreprocessor.GetPermutation(i);

			for (auto& k : perm)
			{
				if (!permutationTable.contains(k))
				{
					SpfMetadataValueKeyPair* kp = new SpfMetadataValueKeyPair(k, std::to_string(permIndex++));
					permutationTable[k] = kp;
					permutationToId[k] = permIndex - 1;
					MetaSize += kp->Size();
				}
			}
		}
		spfMetaHeader.MetadataCount = permutationTable.size();
		spfMetaHeader.MetadataLen = MetaSize;
	}

	JobManager::Dispatch(sourcePreprocessor.GetPermutationCount(), 1, [&](JobDispatchArgs args) {
		int i = args.jobIndex;

		ShaderPreprocessor sourcePreprocessor1;

		sourcePreprocessor1.AddSearchPath("Engine/Include/");

		std::filesystem::path p1(input);
		p1.remove_filename();
		std::string p1s = p1.string();
		sourcePreprocessor1.AddSearchPath(p1s);

		Z_INFO("================ Building DirectX target - Permutation: {} ================", i);
		std::vector<uint32_t> targetDX = GetShaderBinary(ss, sourcePreprocessor1, input, type, 0, sourcePreprocessor.GetPermutation(i), Render::RendererAPI::DX12);

		if (targetDX.empty()) {
			Z_WARN("Error while building SpirVObject {}", input);
			Z_INFO("================ Shader Compilation: 0 successful, 2 errors, 0 updated ================");
			begin = std::chrono::system_clock::now();
			since_epoch = begin.time_since_epoch();
			long end = (long)(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count()) - start;
			Z_INFO("================ Shader Compilation: took {} seconds ================", (float)(end / 1000.0f));
			exit(EXIT_FAILURE);
			return false;
		}

		std::bitset<64> bitset{};

		for (auto str : sourcePreprocessor.GetPermutation(i))
		{
			bitset.set(permutationToId[str]);
		}

		access.acquire();
		access.release();

		std::string name = std::string("dx").append(bitset.to_string());

		SpfFileMetadata* shaderdxMetadata = new SpfFileMetadata(name.append(".shader"));
		fileIndex.FileMetaLen += shaderdxMetadata->Size();

		spirv_cross::CompilerHLSL hlsl(targetDX);
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 50;
		hlsl.set_hlsl_options(options);
		std::string source = hlsl.compile();

		UINT flags = 0;
		flags |= D3DCOMPILE_DEBUG; // add more debug output
#if defined( DEBUG ) || defined( _DEBUG )
#endif

		ID3DBlob* blob_ptr = NULL;
		ID3DBlob* error_blob = NULL;

		HRESULT hr = D3DCompile(
			source.c_str(),
			source.size(),
			input,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"main",
			type == shader_type::vertex ? "vs_5_0" : type == shader_type::fragment ? "ps_5_0" : "cs_5_0",
			flags,
			0,
			&blob_ptr,
			&error_blob);

		if (FAILED(hr)) {
			if (error_blob) {
				printf((char*)error_blob->GetBufferPointer());
				error_blob->Release();
			}
			if (blob_ptr) { blob_ptr->Release(); }
			assert(false);
			exit(-1);
		}

		uint8_t* targetDX_Blob = new uint8_t[blob_ptr->GetBufferSize()];
		memcpy(targetDX_Blob, blob_ptr->GetBufferPointer(), blob_ptr->GetBufferSize());

		shaderdxMetadata->ContentsLen = blob_ptr->GetBufferSize();

		access.acquire();
		fileMetadata.push_back(shaderdxMetadata);
		shaderData.push_back(targetDX_Blob);
		access.release();
		});

	JobManager::Wait();

	uint64_t initialOffset = spfHeader.Size() + spfMetaHeader.Size() + fileIndex.Size() + spfMetaHeader.MetadataLen;

	fileIndex.FileCount = fileMetadata.size();

	for (int i = 0; i < fileMetadata.size(); i++)
	{
		SpfFileMetadata& meta = *fileMetadata[i];
		initialOffset += meta.Size();
	}

	for (int i = 0; i < fileMetadata.size(); i++)
	{
		SpfFileMetadata& meta = *fileMetadata[i];
		meta.ContentIndex = initialOffset;
		initialOffset += meta.ContentsLen;
	}

	spfHeader.Write(out);
	spfMetaHeader.Write(out);

	for (auto kp : permutationTable)
	{
		kp.second->Write(out);
	}

	fileIndex.Write(out);

	for (int i = 0; i < fileMetadata.size(); i++)
	{
		SpfFileMetadata& meta = *fileMetadata[i];
		meta.Write(out);
	}

	for (int i = 0; i < fileMetadata.size(); i++)
	{
		SpfFileMetadata& file = *fileMetadata[i];
		SpfFile::WriteFile((char*)shaderData[i], file.ContentsLen, out, spfHeader);
		delete[] shaderData[i];
	}

	for (int i = 0; i < fileMetadata.size(); i++)
	{
		SpfFileMetadata* file = fileMetadata[i];
		delete file;
	}

	for (auto kp : permutationTable)
	{
		delete kp.second;
	}

	out.close();

	begin = std::chrono::system_clock::now();
	since_epoch = begin.time_since_epoch();
	long end = (long)(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count()) - start;
	Z_INFO("================ Shader Compilation: 2 successful, 0 errors, 0 updated ================");
	Z_INFO("================ Shader Compilation: took {} seconds ================", (float)(end / 1000.0f));

	return true;
}
