#include "ShapeProvider.h"

using namespace ENGINE_NAMESPACE;

Ref<Render::Mesh> g_FullScreenQuad;

const glm::vec3 cube_positions[] =
{
	glm::vec3(10, 10, 10), // DUMMY
	glm::vec3(1, 1, -1),
	glm::vec3(1, -1, -1),
	glm::vec3(1, 1, 1),
	glm::vec3(1, -1, 1),
	glm::vec3(-1, 1, -1),
	glm::vec3(-1, -1, -1),
	glm::vec3(-1, 1, 1),
	glm::vec3(-1, -1, 1),
};

const glm::vec2 cube_uvs[] =
{
	glm::vec2(1, 1), // DUMMY
	glm::vec2(0, 1),
	glm::vec2(1, 0),
	glm::vec2(1, 1),
	glm::vec2(1, 0),
	glm::vec2(0, 1),
	glm::vec2(0, 0),
	glm::vec2(0, 0),
	glm::vec2(1, 1),
};

const uint32_t cube_pos_tris[] =
{
	5, 3, 1,
	3, 8, 4,
	7, 6, 8,
	2, 8, 6,
	1, 4, 2,
	5, 2, 6,
	5, 7, 3,
	3, 7, 8,
	7, 5, 6,
	2, 4, 8,
	1, 3, 4,
	5, 1, 2,
};

const uint32_t cube_uv_tris[] =
{
	1, 2, 3,
	4, 5, 6,
	4, 5, 6,
	4, 5, 6,
	4, 5, 6,
	4, 5, 6,
	1, 7, 2,
	4, 8, 5,
	4, 8, 5,
	4, 8, 5,
	4, 8, 5,
	4, 8, 5,
};

Ref<Render::Mesh> Render::ShapeProvider::GenerateSphere(float radius, int divisions)
{
	float x, y, z, xy;                              // vertex position

	const int stackCount = divisions;
	const int sectorCount = divisions;

	constexpr float pi = 3.1415926f;
	const float sectorStep = 2 * pi / sectorCount;
	const float stackStep = pi / stackCount;
	float sectorAngle, stackAngle;

	std::vector<Render::Vertex> vertices;
	std::vector<uint32_t> indices;

	for (int i = 0; i <= stackCount; ++i)
	{
		stackAngle = pi / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

			Render::Vertex v{};
			v.SetUV(0.0f, 0.0f);
			v.SetNormal(0.0f, 0.0f, 0.0f);
			v.SetTangent(0.0f, 0.0f, 0.0f);
			v.SetPosition(x, y, z);
			memset(v.BoneIds, 255, sizeof(v.BoneIds));

			vertices.push_back(v);

		}
	}

	int k1, k2;
	for (int i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}

		}
	}

	Ref<Mesh> mesh = CreateRef<Mesh>();
	mesh->SetVertices(vertices);
	mesh->SetIndices(indices);
	mesh->CalculateNormals();
	mesh->Update();

	return mesh;
}

Ref<Render::Mesh> Render::ShapeProvider::GenerateCone(const int subdivisions, const float radius, const float height)
{

	std::vector<Render::Vertex> vertices(subdivisions * 2);
	std::vector<uint32_t> indices(subdivisions * 2 * 3);

	vertices[0] = { 0.0f, height, 0.0f, 0.5f, 0.0f };

	for (int i = 0, n = subdivisions - 1; i < subdivisions; i++) {
		float ratio = (float)i / n;
		float r = ratio * (glm::pi<float>() * 2.0f);
		float x = glm::cos(r) * radius;
		float z = glm::sin(r) * radius;
		vertices[i + 1] = { x, height, z, ratio, 0.0f };
	}

	vertices[subdivisions + 1] = { 0.0f, 0.0f, 0.0f , 0.5f, 1.0f };

	for (int i = 0, n = subdivisions - 1; i < n; i++) {
		int offset = i * 3;
		indices[offset] = 0;
		indices[offset + 1] = i + 1;
		indices[offset + 2] = i + 2;
	}

	int bottomOffset = subdivisions * 3;
	for (int i = 0, n = subdivisions - 1; i < n; i++) {
		int offset = i * 3 + bottomOffset;
		indices[offset] = i + 1;
		indices[offset + 1] = subdivisions + 1;
		indices[offset + 2] = i + 2;
	}

	Ref<Mesh> mesh = CreateRef<Mesh>();
	mesh->SetVertices(vertices);
	mesh->SetIndices(indices);
	mesh->CalculateNormals();
	mesh->Update();

	return mesh;
}

Ref<Render::Mesh> Render::ShapeProvider::GenerateBox(const glm::vec3& halfExtents)
{
	std::vector<Render::Vertex> vertices;
	std::vector<uint32_t> indices;

	for (uint32_t i = 0; i < 36U; i++) {
		const glm::vec3& pos = cube_positions[cube_pos_tris[i]];
		const glm::vec2& uv = cube_uvs[cube_uv_tris[i]];
		vertices.push_back({ pos.x, pos.y, pos.z, uv.x, uv.y });
		indices.push_back(i);
	}

	Ref<Mesh> mesh = CreateRef<Mesh>();
	mesh->SetVertices(vertices);
	mesh->SetIndices(indices);
	mesh->CalculateNormals();
	mesh->Update();

	return mesh;
}

Ref<Render::Mesh> Render::ShapeProvider::GenerateQuad(const glm::vec2& size)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices = {
		3, 4, 5, 0, 1, 2
	};

	const float quadVertices[24] = {
		// upper-left triangle
		-size.x, -size.y, 0.0f, 0.0f, // position, texcoord
		-size.x,  size.y, 0.0f, 1.0f,
		 size.x,  size.y, 1.0f, 1.0f,
		 // lower-right triangle
		 -size.x, -size.y, 0.0f, 0.0f,
		  size.x,  size.y, 1.0f, 1.0f,
		  size.x, -size.y, 1.0f, 0.0f
	};

	for (int i = 0; i < 6; i++) {
		Vertex v;
		v.SetPosition(quadVertices[i * 4 + 0], 0.0f, quadVertices[i * 4 + 1]);
		v.SetUV(quadVertices[i * 4 + 2], quadVertices[i * 4 + 3]);
		v.SetNormal(0.0f, 1.0f, 0.0f);
		v.SetTangent(0.0f, 0.0f, 1.0f);
		memset(v.BoneIds, 255, sizeof(v.BoneIds));
		vertices.push_back(v);
	}

	return CreateRef<Mesh>(vertices, indices);
}

Ref<Render::Mesh> Render::ShapeProvider::GenerateFullScreenQuad()
{
	if (g_FullScreenQuad) return g_FullScreenQuad;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices = {
       2, 1, 0
    };

    const float quadVertices[12] = {
        // upper-left triangle
        -1.0f, 1.0f, 0.0f, 0.0f, // position, texcoord
        3.0f, 1.0f, 2.0f, 0.0f,
         -1.0f,  -3.0f, 0.0f, 2.0f,
    };

    for (int i = 0; i < 3; i++) {
        Vertex v;
        v.SetPosition(quadVertices[i * 4 + 0], quadVertices[i * 4 + 1], 0.0f);
        v.SetUV(quadVertices[i * 4 + 2], quadVertices[i * 4 + 3]);
		memset(v.BoneIds, 255, sizeof(v.BoneIds));
        vertices.push_back(v);
    }

	g_FullScreenQuad = CreateRef<Mesh>(vertices, indices);

	return g_FullScreenQuad;
}
