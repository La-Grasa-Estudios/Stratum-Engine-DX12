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

	struct FramebufferAttachment
	{
		ImageResource* pImage = NULL;
		uint32_t subResource = 0; /// SubResource within the image, if the image is a cubemap this specifies what face is used, if the image has an array size greater that 1 it specifies what slice to use (Default = 0)
	};

	struct FramebufferDesc {

		std::vector<FramebufferAttachment> Attachments;

		bool IsWindowSurfaceFb = false;

	};

	/// <summary>
	/// Describes and owns a framebuffer object in the gpu
	/// Strongly recommended to be stored with ThreadSafeResource<Framebuffer>
	/// </summary>
	class Framebuffer {

	public:

		friend class Internal::Window;

		DLLEXPORT Framebuffer(FramebufferDesc description);
		DLLEXPORT ~Framebuffer();

		DLLEXPORT void ClearAttachment(uint32_t attachmentIndex, float col[4]);

		DLLEXPORT glm::ivec2 GetSize();
		DLLEXPORT bool IsWindowFramebuffer();

		FramebufferDesc Desc = {};

	private:

		void Create();

		uint32_t m_Width;
		uint32_t m_Height;
		
		bool m_IsWindowFb;

		std::binary_semaphore lock = std::binary_semaphore(1);

	};

}

END_ENGINE