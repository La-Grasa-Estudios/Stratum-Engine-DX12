#pragma once

#include "znmsp.h"
#include <nvrhi/nvrhi.h>

BEGIN_ENGINE

namespace Render {

	enum class ImageUsage
	{
		STATIC,
		DYNAMIC,

		STAGING,
		RENDERBUFFER, /// Can be used with a framebuffer, cannot be bound to any shader stage
	};

	enum class ImageType
	{
		TEXTURE_2D,
		CUBEMAP_TEXTURE,
	};

	enum class ImageFormat {

		R8_UNORM,
		RG8_UNORM,
		RGBA8_UNORM,

		R8_SNORM,
		RG8_SNORM,
		RGBA8_SNORM,

		R16_UNORM,
		RG16_UNORM,
		RGBA16_UNORM,

		R16_SNORM,
		RG16_SNORM,
		RGBA16_SNORM,

		R16_FLOAT,
		RG16_FLOAT,
		RGBA16_FLOAT,

		R32_FLOAT,
		RG32_FLOAT,
		RGB32_FLOAT,
		RGBA32_FLOAT,

		B5G6R5_UNORM, /// 5 bits red, 6 bits green, 5 bits blue unsigned normalized (0-1) BGR
		R11G11B10_FLOAT, /// 11 bits red, 11 bits green, 10 bits blue floating point

		DEPTH32, /// 32 bits float depth
		DEPTH24, /// 24 bits normalized (0-1) depth
		DEPTH16, /// 16 bits normalized (0-1) depth

		DEPTH24_S8, /// 24 bits depth and 8 bits stencil

		BC7_UNORM,

	};

	class FormatUtil
	{
	public:
		static nvrhi::Format ConvertEngineFormatToNV(const ImageFormat format);
	private:
		static inline nvrhi::Format EngineToNVFormats[] =
		{
			nvrhi::Format::R8_UNORM,
			nvrhi::Format::RG8_UNORM,
			nvrhi::Format::RGBA8_UNORM,

			nvrhi::Format::R8_SNORM,
			nvrhi::Format::RG8_SNORM,
			nvrhi::Format::RGBA8_SNORM,

			nvrhi::Format::R16_UNORM,
			nvrhi::Format::RG16_UNORM,
			nvrhi::Format::RGBA16_UNORM,

			nvrhi::Format::R16_SNORM,
			nvrhi::Format::RG16_SNORM,
			nvrhi::Format::RGBA16_SNORM,

			nvrhi::Format::R16_FLOAT,
			nvrhi::Format::RG16_FLOAT,
			nvrhi::Format::RGBA16_FLOAT,

			nvrhi::Format::R32_FLOAT,
			nvrhi::Format::RG32_FLOAT,
			nvrhi::Format::RGB32_FLOAT,
			nvrhi::Format::RGBA32_FLOAT,

			nvrhi::Format::B5G6R5_UNORM,
			nvrhi::Format::R11G11B10_FLOAT,

			nvrhi::Format::D32,
			nvrhi::Format::D24S8,
			nvrhi::Format::D16,

			nvrhi::Format::D24S8,

			nvrhi::Format::BC7_UNORM,

			nvrhi::Format::BGRA8_UNORM,
		};
	};
}

END_ENGINE