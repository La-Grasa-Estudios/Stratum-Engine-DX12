#pragma once

#include "znmsp.h"
#include <glm/glm.hpp>
#include "Core/NormShort.h"
#include "Core/HalfFloat.h"
#include "Core/Unormint.h"
#include "GraphicsPipeline.h"

BEGIN_ENGINE
namespace Render {
	struct Vertex {

		glm::vec3 position;
		half tx, ty;
		unormbyte4 normal;
		unormbyte4 tangent;

		uint8_t BoneIds[4];
		normshort4 BoneWeights;

		Vertex() {
		}

		Vertex(float x, float y, float z, float u, float v);
		Vertex(float x, float y, float z, float u, float v, float nx_, float ny_, float nz_);

		Vertex(float x, float y, float z);

		void SetUV(float x, float y);
		void SetNormal(float x, float y, float z);
		void SetTangent(float x, float y, float z);
		void SetPosition(float x, float y, float z);

		void SetUV(glm::vec2& v);
		void SetNormal(glm::vec3& v);
		void SetTangent(glm::vec3& v);
		void SetPosition(glm::vec3& v);

		static ShaderVertexLayout GetLayout();

	};
}
END_ENGINE