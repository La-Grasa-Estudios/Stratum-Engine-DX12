#pragma once

#include "znmsp.h"

#include "Renderer/ImageResource.h"
#include "Core/Ref.h"
#include "glm/ext.hpp"

BEGIN_ENGINE

namespace Render {

	enum class MaterialBlend
	{
		ALPHA_TO_COVERAGE,
		ALPHA_BLENDED,
	};

	struct MaterialTexture
	{

		static inline const float DEFAULT_ON_LOAD_LOD = 0.1f;

		Ref<ImageResource> BaseImage;
		Ref<ImageResource> HqImage;
		int32_t BindlessIndex = -1;

		std::string PathToTextureFile = "";
		float CurrentLod = DEFAULT_ON_LOAD_LOD;

		bool IsSamplerTransparent = false;

		MaterialTexture();

		MaterialTexture(Ref<ImageResource> s, bool t);

		ImageResource* GetTexture();


	};

	class Material {
	public:

		glm::vec4  albedo = { 1.0f, 1.0f, 1.0f, 0.0f };
		glm::vec4  m_r_a = { 0.5f, 0.2f, 0.0f, 0.0f };
		glm::vec3  emission = { 0.0f, 0.0f, 0.0f };

		glm::vec2 UvOffset = { 0.0f, 0.0f };
		glm::vec2 UvSpeed = { 0.0f, 0.0f };

		Material();
		~Material();

		bool CastShadows = true;
		bool TwoSided = false;

		MaterialTexture Diffuse = {};
		MaterialTexture Normal = {};
		MaterialTexture Roughness = {};
		MaterialTexture Metalness = {};
		MaterialTexture AO = {};
		MaterialTexture Specular = {};

		float HighestLod = 0.0f;
		bool VisibleThisFrame = false;

		MaterialBlend BlendMode = MaterialBlend::ALPHA_TO_COVERAGE;

		void ComputeHash();

		size_t Hash;

	};
}

END_ENGINE