#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "ImageResource.h"

#include <vector>
#include <glm/glm.hpp>
#include <semaphore>

BEGIN_ENGINE

namespace Internal {
	class Window;
}

namespace Render {

	// Framebuffer attachment
	struct FramebufferAttachment
	{
		ImageResource* pImage = NULL; // The image pointer to be set as the attachment, mush be created with AllowFramebufferUsage set to true
		nvrhi::TextureSubresourceSet subResourceSet = nvrhi::AllSubresources; // SubResource within the image, if the image is a cubemap this specifies what face is used, if the image has an array size greater that 1 it specifies what slice to use (Default = 0)
	};

	// Framebuffer creation desc
	struct FramebufferDesc {

		std::vector<FramebufferAttachment> Attachments;

		bool IsWindowSurfaceFb = false; // Internal use only TO DO: Remove this :)

	};

	/// <summary>
	/// Describes and owns a framebuffer object in the gpu
	/// </summary>
	class Framebuffer {

	public:

		friend class Internal::Window;

		Framebuffer(FramebufferDesc description);
		~Framebuffer();

		glm::ivec2 GetSize();
		bool IsWindowFramebuffer();

		FramebufferDesc Desc = {};
		nvrhi::FramebufferHandle Handle;

	private:

		void Create();

		uint32_t m_Width;
		uint32_t m_Height;
		
		bool m_IsWindowFb;

		std::binary_semaphore lock = std::binary_semaphore(1);

	};

}

END_ENGINE