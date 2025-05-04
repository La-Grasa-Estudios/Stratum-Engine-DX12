#pragma once

#include "ConstantBuffer.h"
#include "Framebuffer.h"

#include <sstream>

#define INSTANCE_COPY(dst, src, type, offset) memcpy(dst.buffer + offset, &src, sizeof(type));

BEGIN_ENGINE

namespace Render {

	enum class ShaderVisibility
	{
		VERTEX,
		PIXEL,
		COMPUTE,
	};

	enum class RasterizerFillMode {
		FILL = 0,
		WIREFRAME = 1,
	};

	enum class RCullMode {
		NOT = 2,
		FRONT = 1,
		BACK = 0,
	};

	struct RasterizerState {
		RasterizerFillMode FillMode = RasterizerFillMode::FILL;
		RCullMode CullMode = RCullMode::BACK;
		bool DepthTest = true;
		bool EnableScissor = false;
		void* PipelineObject = NULL; // Do not modify this!, its used by the internal render pipeline
	};

	enum class Blend {
		ZERO = 1,
		ONE = 2,
		SRC_COLOR = 3,
		INV_SRC_COLOR = 4,
		SRC_ALPHA = 5,
		INV_SRC_ALPHA = 6,
		DEST_ALPHA = 7,
		INV_DEST_ALPHA = 8,
		DEST_COLOR = 9,
		INV_DEST_COLOR = 10,
		SRC_ALPHA_SAT = 11,
		BLEND_FACTOR = 14,
		INV_BLEND_FACTOR = 15,
		SRC1_COLOR = 16,
		INV_SRC1_COLOR = 17,
		SRC1_ALPHA = 18,
		INV_SRC1_ALPHA = 19
	};

	enum class BlendOp {
		ADD = 1,
		SUBTRACT = 2,
		REV_SUBTRACT = 3,
		MIN = 4,
		MAX = 5
	};

	enum class ComparisonFunction {
		NEVER = 1,
		LESS = 2,
		EQUAL = 3,
		LESS_EQUAL = 4,
		GREATER = 5,
		NOT_EQUAL = 6,
		GREATER_EQUAL = 7,
		ALWAYS = 8
	};

	struct RenderTargetBlendState {
		bool EnableBlend = true;
		Blend SrcBlend = Blend::SRC_ALPHA;
		Blend DestBlend = Blend::INV_SRC_ALPHA;
		Blend SrcBlendAlpha = Blend::SRC_ALPHA;
		Blend DestBlendAlpha = Blend::ONE;
		BlendOp BlendOperator = BlendOp::ADD;
		BlendOp BlendOperatorAlpha = BlendOp::ADD;
	};

	struct BlendState {
		bool IndependentBlendEnable = false;
		RenderTargetBlendState BlendStates[8]{};
		void* PipelineObject = NULL; // Do not modify this!, its used by the internal render pipeline
	};

	enum class StencilOperation {
		OP_KEEP = 1,
		OP_ZERO = 2,
		OP_REPLACE = 3,
		OP_INCR_SAT = 4,
		OP_DECR_SAT = 5,
		OP_INVERT = 6,
		OP_INCR = 7,
		OP_DECR = 8
	};

	struct StencilOperationDescription {
		StencilOperation StencilFailOp = StencilOperation::OP_KEEP;
		StencilOperation StencilDepthFailOp = StencilOperation::OP_KEEP;
		StencilOperation StencilPassOp = StencilOperation::OP_KEEP;
		ComparisonFunction StencilFunc = ComparisonFunction::ALWAYS;
	};

	struct StencilState {
		bool DepthEnable = true;
		bool DepthEnableWriting = true;
		ComparisonFunction DepthFunc = ComparisonFunction::LESS_EQUAL;
		bool StencilEnable = false;
		uint8_t StencilReadMask = 255;
		uint8_t StencilWriteMask = 255;
		StencilOperationDescription FrontFace;
		StencilOperationDescription BackFace;
		float ClearDepth = 1.0f;
		int StencilRef = 0;
		void* PipelineObject = NULL; // Do not modify this!, its used by the internal render pipeline
	};

	/// <summary>
	/// Defines a render viewport, if width and height == 0 the render pipelines uses the specified framebuffer size
	/// </summary>
	struct Viewport {

		int32_t x = 0;
		int32_t y = 0;
		int32_t width = 0;
		int32_t height = 0;

	};

	typedef std::stringstream InmediateModeList;

}

END_ENGINE