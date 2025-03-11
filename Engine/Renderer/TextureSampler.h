#pragma once

#include <string>

#include "ImageResource.h"
#include "Texture3D.h"

#include "Core/Ref.h"

#include "znmsp.h"

BEGIN_ENGINE
	
namespace Render {

	enum class TextureFilterMode {
		POINT,
		BILINEAR,
	};

	enum class TextureWrapMode
	{
		REPEAT,
		CLAMP,
	};

	class TextureSampler
	{

	public:

		TextureFilterMode filterMode = TextureFilterMode::BILINEAR;

		TextureSampler(Ref<ImageResource> image);
		TextureSampler(Ref<Texture3D> texture3d);

		void SetFilter(TextureFilterMode mode);
		void SetWrapMode(TextureWrapMode mode);
		void SetLod(float minLod, float maxLod);
		glm::vec2 GetLod();
		void* GetRendererData();

		void SetImage(Ref<ImageResource> image);
		
		Ref<ImageResource> GetImage();
		Ref<Texture3D> GetTexture();
		~TextureSampler();

	private:

		Ref<ImageResource> m_ImageRef;
		Ref<Texture3D> m_TextureRef;

	};

}

END_ENGINE