#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glm/ext.hpp>

#include "znmsp.h"

#include "Core/Ref.h"
#include "RenderCommands.h"


BEGIN_ENGINE
namespace Render {

	enum class VertexType {

		INT,
		SHORT,
		BYTE,

		INT4_16_NORM,
		INT2_16_NORM,
		INT16_NORM,
		INT4_8_NORM,
		INT2_8_NORM,
		INT8_NORM,

		UINT4_16_NORM,
		UINT2_16_NORM,
		UINT16_NORM,
		UINT4_8_NORM,
		UINT2_8_NORM,
		UINT8_NORM,

		UBYTE4,

		UINT,
		USHORT,
		UBYTE,

		FLOAT_32,
		FLOAT2_32,
		FLOAT3_32,
		FLOAT4_32,

		FLOAT_16,
		FLOAT2_16,
		FLOAT4_16,

		FLOAT3_11_11_10,

	};

	enum class VertexInputRate {
		PER_VERTEX,
		PER_INSTANCE,
	};

	struct VertexAttribute {

		VertexType Type;
		VertexInputRate InputRate = VertexInputRate::PER_VERTEX;
		uint32_t Offset;
		uint32_t Index;
		bool Normalized;

	};

	struct ShaderVertexLayout {
		std::vector<VertexAttribute> VertexAttributes;
	};

	/// Describes a root constant on d3d12 or a push constant on vulkan
	/// Size must be a multiple of 4!
	struct ShaderConstants
	{
		size_t size;
		uint32_t index;
		ShaderVisibility visibility;
		ShaderConstants(size_t Size, uint32_t Index, ShaderVisibility Vis);
	};

	struct PipelineDescription {
		std::string VertexShaderPath;
		std::string PixelShaderPath;
		std::string GeometryShaderPath;

		BlendState BlendState;
		RasterizerState RasterizerState;
		StencilState StencilState;
		ShaderVertexLayout VertexLayout;
		uint32_t NumRenderTargets;
		ImageFormat RenderTargetFormats[8] = {};
		ImageFormat DepthTargetFormat = ImageFormat::DEPTH16;

		std::vector<ShaderConstants> Constants;

		void RequirePermutation(const std::string& key);

		std::string PermutationKey;

		DLLEXPORT PipelineDescription();

		DLLEXPORT void FromFile(std::string_view path);
	};


	struct PipelineCreationDescription {
		std::vector<uint8_t> SourceVertex;
		std::vector<uint8_t> SourcePixel;
		std::vector<uint8_t> SourceGeometry;
		PipelineDescription Description;
	};

    class GraphicsPipeline
    {
    public:

        PipelineDescription ShaderDesc;

        GraphicsPipeline(PipelineDescription desc);
        ~GraphicsPipeline();
		
		template<typename T>
		T* As()
		{
			return (T*)NativePointer;
		}

	private:

		void* NativePointer;
    };
}
END_ENGINE