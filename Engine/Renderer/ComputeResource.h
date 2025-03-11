#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "RendererContext.h"
#include "Buffer.h"
#include "Texture3D.h"

BEGIN_ENGINE

namespace Render {

	class ComputeResource {

	public:

		DLLEXPORT ComputeResource(Buffer* buffer);
		DLLEXPORT ComputeResource(ImageResource* image, uint32_t mipLevel, uint32_t arraySlice);
		DLLEXPORT ComputeResource(Texture3D* texture);

	};

}

END_ENGINE