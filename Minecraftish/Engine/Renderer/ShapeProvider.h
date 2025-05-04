#pragma once

#include "Mesh.h"

BEGIN_ENGINE

namespace Render
{
	class ShapeProvider
	{
	public:

		static Ref<Mesh> GenerateSphere(float radius, int divisions);
		static Ref<Render::Mesh> GenerateCone(const int subdivisions, const float radius, const float height);
		static Ref<Mesh> GenerateBox(const glm::vec3& halfExtents);
		static Ref<Mesh> GenerateQuad(const glm::vec2& size);
		static Ref<Mesh> GenerateFullScreenQuad();

	};
}

END_ENGINE