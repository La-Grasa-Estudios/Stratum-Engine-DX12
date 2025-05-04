#pragma once

#include <string>

#include "Core/Ref.h"

#include "znmsp.h"

#include <nvrhi/nvrhi.h>

BEGIN_ENGINE
	
namespace Render {

	enum class TextureFilterMode {
		POINT,
		BILINEAR,
		ANISOTROPIC,
	};

	enum class TextureWrapMode
	{
		CLAMP,
		WRAP,
	};

	struct TextureSamplerDescription
	{
		TextureFilterMode Filter = TextureFilterMode::BILINEAR;
		TextureWrapMode AddressMode = TextureWrapMode::WRAP;
		float MipBias = 0.0f;
		float AnisoLevel = 1.0f;
	};

	class TextureSampler
	{

	public:

		TextureSampler(const TextureSamplerDescription& desc);
		~TextureSampler();

		TextureSamplerDescription SamplerDesc;

		nvrhi::SamplerHandle Handle;

	private:


	};

}

END_ENGINE