#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "GraphicsFormats.h"

#include <vector>
#include <nvrhi/nvrhi.h>

#include <glm/ext.hpp>

BEGIN_ENGINE

namespace Render {

	struct ImageResourceData
	{
		void* pSysMem;
		uint32_t MemPitch;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="ptr">Pointer to main memory</param>
		/// <param name="pitch">Width * bytes per pixel</param>
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
		ImageType Type = ImageType::TEXTURE_2D;

		bool Immutable = false;
		glm::vec4 ClearValue = { 1.0f, 1.0f, 1.0f, 1.0f };

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

		ImageResource() = default;
		ImageResource(const ImageDescription& desc);
		~ImageResource();

		glm::ivec2 GetSize();

		bool IsResourceReady();

		nvrhi::TextureHandle Handle;

		ImageDescription ImageDesc;

	private:

		bool mResourceReady = false;

	};

}

END_ENGINE