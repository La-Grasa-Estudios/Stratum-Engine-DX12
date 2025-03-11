#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "GraphicsFormats.h"

#include <vector>

#include <glm/ext.hpp>

BEGIN_ENGINE

namespace Render {

	struct ImageResourceData
	{
		void* pSysMem;
		uint32_t MemPitch;

		ImageResourceData(void* ptr, uint32_t pitch)
		{
			pSysMem = ptr;
			MemPitch = pitch;
		}
	};

	struct ImageDescription
	{

		uint32_t Width;
		uint32_t Height;

		uint32_t MipLevels = 1;
		uint32_t ArraySize = 1;

		ImageFormat Format;
		ImageUsage Usage = ImageUsage::STATIC;
		ImageType Type = ImageType::TEXTURE_2D;

		bool AllowWritesFromMainMemory = false;
		bool AllowReadsToMainMemory = false;
		bool AllowFramebufferUsage = false;
		bool AllowComputeResourceUsage = false;

		std::vector<ImageResourceData> DefaultData;

		bool IsDepthFormat() const
		{
			return Format >= ImageFormat::DEPTH32 && Format <= ImageFormat::DEPTH24_S8;
		}

	};

	class ImageResource {



	public:

		DLLEXPORT ImageResource(const ImageDescription& desc);

		DLLEXPORT void* Map();
		DLLEXPORT void Unmap();
		DLLEXPORT glm::ivec2 GetSize();

		ImageDescription ImageDesc;

	private:



	};

}

END_ENGINE