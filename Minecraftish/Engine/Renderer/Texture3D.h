#pragma once

#include "znmsp.h"

#include "Core/Ref.h"

#include "GraphicsFormats.h"

#include <glm/ext.hpp>

BEGIN_ENGINE

namespace Render
{

	struct Texture3DDescription
	{

		uint32_t Width;
		uint32_t Height;
		uint32_t Depth;

		uint32_t MipLevels = 1;

		ImageFormat Format;

		bool AllowWritesFromMainMemory = false;
		bool AllowReadsToMainMemory = false;
		bool AllowComputeResourceUsage = false;

		//std::vector<ImageResourceData> DefaultData;

		bool IsDepthFormat() const
		{
			return Format >= ImageFormat::DEPTH32 && Format <= ImageFormat::DEPTH24_S8;
		}

	};

	class Texture3D
	{

	public:

		Texture3D(const Texture3DDescription& desc);
		~Texture3D();

		glm::ivec3 GetSize();

		nvrhi::TextureHandle Handle;
		Texture3DDescription TextureDesc;

	};

}

END_ENGINE