#include "ShaderCompiler.h"

#include <atlbase.h> 
#include <dxc/dxcapi.h>
#include <dxc/d3d12shader.h>
#include <codecvt>
#include <unordered_set>
#include <wrl/client.h>

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxil.lib")

using namespace ENGINE_NAMESPACE;

struct ZVFSDxcIncludeHandler : public IDxcIncludeHandler
{
	CComPtr<IDxcUtils> pUtils;

	ZVFSDxcIncludeHandler()
	{
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	}

	HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR pFilename,
		_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string sstring = converter.to_bytes(pFilename);

		{
			std::string nstring;
			sstring = sstring.substr(2);
			for (int i = 0; i < sstring.size(); i++)
			{
				char c = sstring[i];
				if (c == '\\')
				{
					c = '/';
				}
				nstring += c;
			}
			sstring = nstring;
		}

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> pEncoding;

		if (s_Included.contains(sstring))
		{
			static const char nullStr[] = " ";
			pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, pEncoding.GetAddressOf());
			*ppIncludeSource = pEncoding.Detach();
			return S_OK;
		}

		RefBinaryStream ss = ZVFS::GetFile(sstring.c_str());

		if (ss->Size() != 0)
		{
			s_Included.insert(sstring);

			std::filesystem::path p1(sstring);
			p1.remove_filename();
			std::string p1s = p1.string();
			AddSearchPath(p1s);

			HRESULT hr = pUtils->CreateBlob(ss->As<char>(), ss->Size(), DXC_CP_ACP, pEncoding.GetAddressOf());

			*ppIncludeSource = pEncoding.Detach();

			return hr;
		}
		else
		{
			std::string includePath;
			std::string code = OpenFile(sstring.c_str(), includePath);

			if (!code.empty())
			{
				s_Included.insert(sstring);

				std::filesystem::path p1(includePath);
				p1.remove_filename();
				std::string p1s = p1.string();
				AddSearchPath(p1s);

				HRESULT hr = pUtils->CreateBlob(code.data(), code.size(), DXC_CP_ACP, pEncoding.GetAddressOf());

				*ppIncludeSource = pEncoding.Detach();

				return hr;
			}
		}

		*ppIncludeSource = NULL;
		return ERROR_FILE_NOT_FOUND;
	}

	void AddSearchPath(const std::string& path)
	{
		m_SearchPaths.push_back(path);
	}

	std::string OpenFile(const char* file, std::string& fullpath)
	{
		if (ENGINE_NAMESPACE::ZVFS::Exists(file))
		{
			ENGINE_NAMESPACE::RefBinaryStream stream = ENGINE_NAMESPACE::ZVFS::GetFile(file);
			return stream->Str();
		}

		for (int i = 0; i < m_SearchPaths.size(); i++)
		{
			std::string fullPath = m_SearchPaths[i];
			fullPath.append(file);
			if (ENGINE_NAMESPACE::ZVFS::Exists(fullPath.c_str()))
			{
				fullpath.clear();
				fullpath.append(fullPath);
				ENGINE_NAMESPACE::RefBinaryStream stream = ENGINE_NAMESPACE::ZVFS::GetFile(fullPath.c_str());
				return stream->Str();
			}
		}

		Z_ERROR("Failed to open include file: {}", file);

		return "";
	}

	std::unordered_set<std::string> s_Included;

	std::vector<std::string> m_SearchPaths;
	std::vector<std::vector<std::string>> m_Permutations;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	ULONG STDMETHODCALLTYPE Release(void) override { return 0; }
};

static ShaderReflectionResourceDimension ConvertD3DDimensionToEngine(D3D_SRV_DIMENSION dimension)
{
	switch (dimension)
	{
	default:
	case D3D_SRV_DIMENSION_UNKNOWN:
		return ShaderReflectionResourceDimension::UNKNOWN;
	case D3D_SRV_DIMENSION_BUFFER:
		return ShaderReflectionResourceDimension::BUFFER;
	case D3D_SRV_DIMENSION_TEXTURE1D:
		return ShaderReflectionResourceDimension::TEXTURE1D;
	case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
		return ShaderReflectionResourceDimension::TEXTURE1DARRAY;
	case D3D_SRV_DIMENSION_TEXTURE2D:
		return ShaderReflectionResourceDimension::TEXTURE2D;
	case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
		return ShaderReflectionResourceDimension::TEXTURE2DARRAY;
	case D3D_SRV_DIMENSION_TEXTURE3D:
		return ShaderReflectionResourceDimension::TEXTURE3D;
	case D3D_SRV_DIMENSION_TEXTURECUBE:
		return ShaderReflectionResourceDimension::TEXTURECUBE;
	case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
		return ShaderReflectionResourceDimension::TEXTURECUBEARRAY;
	}
}

std::vector<uint8_t> GLSLToSpirV::GetShaderBinary(RefBinaryStream& ss, ShaderPreprocessor& processor, const char* input, GLSLToSpirV::shader_type type, int permIndex, std::vector<std::string> defines, std::vector<shaderbinding_t>& shaderBindings, Render::RendererAPI targetApi)
{

	using namespace Microsoft::WRL;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring inputStr = converter.from_bytes(input);

	std::string stage = "STAGE_";
	LPCWSTR shaderTarget = NULL;

	switch (type)
	{
	case GLSLToSpirV::shader_type::fragment:
		stage.append("PIXEL");
		shaderTarget = L"ps_6_0";
		break;
	case GLSLToSpirV::shader_type::vertex:
		stage.append("VERTEX");
		shaderTarget = L"vs_6_0";
		break;
		stage.append("GEOMETRY");
		shaderTarget = L"gs_6_0";
		break;
	case GLSLToSpirV::shader_type::compute:
		stage.append("COMPUTE");
		shaderTarget = L"cs_6_0";
		break;
	default:
		break;
	}

	defines.push_back(g_Apis[(int)targetApi]);
	defines.push_back(std::string("PERM_").append(std::to_string(permIndex)));
	defines.push_back(stage);

	CComPtr<IDxcUtils> pUtils;
	CComPtr<IDxcCompiler3> pCompiler;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

	ZVFSDxcIncludeHandler* IncludeHandler = new ZVFSDxcIncludeHandler();
	CComPtr<IDxcIncludeHandler> pIncludeHandler;
	pIncludeHandler.Attach(IncludeHandler);

	IncludeHandler->AddSearchPath("Engine/Include/");

	std::vector<LPCWSTR> pszArgs =
	{
		inputStr.c_str(),            // Optional shader source file name for error reporting
		L"-E", L"main",              // Entry point.
		L"-T", shaderTarget,            // Target.
		L"-I", L"Engine/Include",            // Target.
		L"-Zi",                      // Enable debug information (slim format)
		L"-Qembed_debug",                      // Enable debug information (slim format)
	};

	for (auto s1 : defines)
	{
		std::wstring wstr = converter.from_bytes(s1);
		size_t sz = sizeof(WCHAR) * (wstr.size() + 1);
		WCHAR* strb = (WCHAR*)malloc(sz);
		memset(strb, 0, sz);
		memcpy(strb, wstr.c_str(), sz - 1);
		pszArgs.push_back(L"-D");
		pszArgs.push_back(strb);
	}

	std::string rawCode = ss->Str();

	std::string code = processor.PreProcessSource(rawCode);

	CComPtr<IDxcBlobEncoding> pSource = nullptr;
	pUtils->LoadFile(inputStr.c_str(), nullptr, &pSource);
	DxcBuffer Source;
	Source.Ptr = code.data();
	Source.Size = code.size();
	Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

	// L"-D", L"MYDEFINE=1",

	CComPtr<IDxcResult> pResults;
	pCompiler->Compile(
		&Source,                // Source buffer.
		pszArgs.data(),                // Array of pointers to arguments.
		pszArgs.size(),      // Number of arguments.
		pIncludeHandler,        // User-provided interface to handle #include directives (optional).
		IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
	);

	CComPtr<IDxcBlobUtf8> pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	// Note that d3dcompiler would return null if no errors or warnings are present.
	// IDxcCompiler3::Compile will always return an error buffer, but its length
	// will be zero if there are no warnings or errors.
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
		wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());

	HRESULT hrStatus;
	pResults->GetStatus(&hrStatus);
	if (FAILED(hrStatus))
	{
		wprintf(L"Compilation Failed\n");
		exit(-1);
	}

	CComPtr<IDxcBlob> pShader = nullptr;
	CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
	pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
	if (pShader != nullptr)
	{
		std::vector<uint8_t> compiledShader;

		for (size_t i = 0; i < pShader->GetBufferSize(); i++)
		{
			compiledShader.push_back(reinterpret_cast<uint8_t*>(pShader->GetBufferPointer())[i]);
		}

		ComPtr<IDxcBlob> reflectionBlob{};
		pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);

		const DxcBuffer reflectionBuffer
		{
			.Ptr = reflectionBlob->GetBufferPointer(),
			.Size = reflectionBlob->GetBufferSize(),
			.Encoding = 0,
		};

		ComPtr<ID3D12ShaderReflection> shaderReflection{};
		pUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		D3D12_SHADER_DESC shaderDesc{};
		shaderReflection->GetDesc(&shaderDesc);

		for (int i = 0; i < shaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
			shaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc);

			if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::CBUFFER;
				binding.Dimension = ShaderReflectionResourceDimension::BUFFER;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::SAMPLER;
				binding.Dimension = ShaderReflectionResourceDimension::UNKNOWN;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::TEXTURE;
				binding.Dimension = ShaderReflectionResourceDimension::UNKNOWN;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_BYTEADDRESS)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::BYTEADDRESS;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.Dimension = ConvertD3DDimensionToEngine(shaderInputBindDesc.Dimension);
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::STRUCTURED_BUFFER;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.Dimension = ConvertD3DDimensionToEngine(shaderInputBindDesc.Dimension);
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::UAV_RW_BYTEADDRESS;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.Dimension = ConvertD3DDimensionToEngine(shaderInputBindDesc.Dimension);
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::UAV_RW_STRUCTURED;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.Dimension = ConvertD3DDimensionToEngine(shaderInputBindDesc.Dimension);
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
			{
				shaderbinding_t binding{};

				binding.Type = ShaderReflectionResourceType::UAV_RW_TYPED;
				binding.BindingPoint = shaderInputBindDesc.BindPoint;
				binding.Dimension = ConvertD3DDimensionToEngine(shaderInputBindDesc.Dimension);
				binding.BindingSpace = shaderInputBindDesc.Space;

				shaderBindings.push_back(binding);
			}

			Z_INFO(std::to_string((int)shaderInputBindDesc.Type));
		}

		return compiledShader;
	}

	return std::vector<uint8_t>();
}

bool GLSLToSpirV::build_object(const char* input, const char* output, shader_type type, int nbPermutations)
{
	RefBinaryStream ss = ENGINE_NAMESPACE::ZVFS::GetFile(input);

	if (ss->Str().empty()) return false;

	if (std::string(input).ends_with("glsl"))
	{
		Z_WARN("GLSL Is no longer supported!");
		return false;
	}

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

		if (type == shader_type::fragment || type == shader_type::vertex)
		{
			shader_type types[] =
			{
				shader_type::vertex,
				shader_type::fragment,
			};
			for (int k = 0; k < 2; k++)
			{
				type = types[k];

				std::vector<shaderbinding_t> shaderBindings;
				std::vector<uint8_t> targetDX = GetShaderBinary(ss, sourcePreprocessor1, input, type, 0, sourcePreprocessor.GetPermutation(i), shaderBindings, Render::RendererAPI::DX12);

				std::stringstream shaderBindingStream;

				shaderreflection_t reflect
				{
					.NumShaderBidings = (uint32_t)shaderBindings.size(),
				};

				shaderBindingStream.write((char*)&reflect.NumShaderBidings, sizeof(uint32_t));

				for (int i = 0; i < shaderBindings.size(); i++)
				{
					shaderbinding_t binding = shaderBindings[i];
					shaderBindingStream.write((char*)&binding.Type, sizeof(uint8_t));
					shaderBindingStream.write((char*)&binding.Dimension, sizeof(uint8_t));
					shaderBindingStream.write((char*)&binding.BindingPoint, sizeof(uint8_t));
					shaderBindingStream.write((char*)&binding.BindingSpace, sizeof(uint8_t));
				}

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

				const char* dxNames[] =
				{
					"dx11",
					"dx12",
				};

				std::string stageName = "pixel";

				if (type == shader_type::vertex)
				{
					stageName = "vertex";
				}

				if (type == shader_type::compute)
				{
					stageName = "comp";
				}

				std::string SigBlob = shaderBindingStream.str();

				for (int i = 0; i < 2; i++)
				{
					std::string name = std::string(dxNames[i]).append(stageName).append(bitset.to_string());
					std::string rootsigname = std::string(name).append(".sig");

					SpfFileMetadata* shaderdxMetadata = new SpfFileMetadata(name.append(".shader"));
					fileIndex.FileMetaLen += shaderdxMetadata->Size();

					SpfFileMetadata* sigDxMetadata = new SpfFileMetadata(rootsigname);
					fileIndex.FileMetaLen += sigDxMetadata->Size();

					uint8_t* targetDX_Blob = new uint8_t[targetDX.size()];
					memcpy(targetDX_Blob, targetDX.data(), targetDX.size());

					uint8_t* pSigBlob = new uint8_t[SigBlob.size()];
					memcpy(pSigBlob, SigBlob.data(), SigBlob.size());

					shaderdxMetadata->ContentsLen = targetDX.size();
					sigDxMetadata->ContentsLen = SigBlob.size();

					access.acquire();
					fileMetadata.push_back(shaderdxMetadata);
					fileMetadata.push_back(sigDxMetadata);
					shaderData.push_back(targetDX_Blob);
					shaderData.push_back(pSigBlob);
					access.release();
				}
			}
		}
		else
		{
			std::vector<shaderbinding_t> shaderBindings;
			std::vector<uint8_t> targetDX = GetShaderBinary(ss, sourcePreprocessor1, input, type, 0, sourcePreprocessor.GetPermutation(i), shaderBindings, Render::RendererAPI::DX12);

			std::stringstream shaderBindingStream;

			shaderreflection_t reflect
			{
				.NumShaderBidings = (uint32_t)shaderBindings.size(),
			};

			shaderBindingStream.write((char*)&reflect.NumShaderBidings, sizeof(uint32_t));

			for (int i = 0; i < shaderBindings.size(); i++)
			{
				shaderbinding_t binding = shaderBindings[i];
				shaderBindingStream.write((char*)&binding.Type, sizeof(uint8_t));
				shaderBindingStream.write((char*)&binding.Dimension, sizeof(uint8_t));
				shaderBindingStream.write((char*)&binding.BindingPoint, sizeof(uint8_t));
				shaderBindingStream.write((char*)&binding.BindingSpace, sizeof(uint8_t));
			}

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

			const char* dxNames[] =
			{
				"dx11",
				"dx12",
			};

			std::string SigBlob = shaderBindingStream.str();

			for (int i = 0; i < 2; i++)
			{
				std::string name = std::string(dxNames[i]).append("comp").append(bitset.to_string());
				std::string rootsigname = std::string(name).append(".sig");

				SpfFileMetadata* shaderdxMetadata = new SpfFileMetadata(name.append(".shader"));
				fileIndex.FileMetaLen += shaderdxMetadata->Size();

				SpfFileMetadata* sigDxMetadata = new SpfFileMetadata(rootsigname);
				fileIndex.FileMetaLen += sigDxMetadata->Size();

				uint8_t* targetDX_Blob = new uint8_t[targetDX.size()];
				memcpy(targetDX_Blob, targetDX.data(), targetDX.size());

				uint8_t* pSigBlob = new uint8_t[SigBlob.size()];
				memcpy(pSigBlob, SigBlob.data(), SigBlob.size());

				shaderdxMetadata->ContentsLen = targetDX.size();
				sigDxMetadata->ContentsLen = SigBlob.size();

				access.acquire();
				fileMetadata.push_back(shaderdxMetadata);
				fileMetadata.push_back(sigDxMetadata);
				shaderData.push_back(targetDX_Blob);
				shaderData.push_back(pSigBlob);
				access.release();
			}
		}
		

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
