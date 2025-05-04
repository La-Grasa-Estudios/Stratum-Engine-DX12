#pragma once

#include "znmsp.h"
#include "Renderer/ImageResource.h"
#include "AssetMaterial.h"

BEGIN_ENGINE

namespace Render
{

	enum class TextureCreationType
	{
		RGBA,
		BC7,
		CTEX,
	};

	enum class TextureFormat {

		R8,
		RG8,
		RGB8,
		RGBA8,

	};

	struct TextureCreationRGBA
	{
		TextureFormat Format;
		void* pSysMem;
	};

	struct TextureCreationBC7
	{
		uint32_t bc7Header[9]{};
		void* pSysMem;
	};

	struct TextureCreationCTEX
	{
		textureheader_t header;
		std::istream* pStream;
	};

	struct ITextureDescription
	{
		TextureCreationType Type = TextureCreationType::RGBA;

		union
		{
			TextureCreationRGBA RGBA;
			TextureCreationBC7 BC7;
			TextureCreationCTEX CTEX;
		};

		uint32_t Width;
		uint32_t Height;
		bool Streamable;
	};

	class TextureLoader
	{

	public:

		static Ref<ImageResource> LoadFileToImage(const std::string& path, bool* bIsTransparent, float fRequestedLevelOfDetail = 1.0f);

	};
}

END_ENGINE