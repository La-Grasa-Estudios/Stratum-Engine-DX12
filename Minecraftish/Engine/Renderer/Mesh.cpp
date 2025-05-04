#include "Mesh.h"

#include "RendererContext.h"
#include "Core/HalfFloat.h"

using namespace ENGINE_NAMESPACE;

Render::Mesh::Mesh()
{

}

Render::Mesh::~Mesh()
{

}

Render::Mesh::Mesh(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	Update(vertices, indices);
}

Render::Mesh::Mesh(const std::string& mfFile)
{

}

void Render::Mesh::Update(std::vector<Vertex>& t_vertices, const std::vector<uint32_t>& t_indices)
{

	this->vertices = t_vertices;
	this->indices = t_indices;

	Update();

}

void Render::Mesh::Update()
{

	if (vertices.empty()) return;

	constexpr float ep = 9999999.0f;
	constexpr float en = -ep;

	this->Min = { ep, ep, ep };
	this->Max = { en, en, en };

	size_t hash = 0;

	for (int i = 0; i < vertices.size(); i++) {

		Vertex& vertex = vertices[i];

		glm::vec3& pos = vertex.position;

		hash ^= std::hash<glm::vec3>::_Do_hash(pos);

		this->Min = glm::min(pos, this->Min);
		this->Max = glm::max(pos, this->Max);

	}

	for (int i = 0; i < indices.size(); i++)
	{
		hash ^= std::hash<uint32_t>()(indices[i]);
	}

	Hash = hash;

	Centre = (Max + Min) / 2.0f;
	Radius = glm::length(Max - Min);

	BufferDescription vertexBd{};

	vertexBd.pSysMem = vertices.data();
	vertexBd.Size = sizeof(Vertex) * vertices.size();

	vertexBd.Immutable = !IsRigged;
	vertexBd.Type = BufferType::VERTEX_BUFFER;
	vertexBd.AllowComputeResourceUsage = IsRigged;
	vertexBd.StructuredStride = sizeof(Render::Vertex);

	BufferDescription indexBd{};

	indexBd.Immutable = !IsRigged;
	indexBd.pSysMem = indices.data();
	indexBd.Size = sizeof(uint32_t) * indices.size();

	indexBd.Type = BufferType::INDEX_BUFFER;

	m_VertexBufferHandle = CreateRef<Buffer>(vertexBd);
	m_IndexBufferHandle = CreateRef<Buffer>(indexBd);

	m_VertexBuffer = CreateRef<VertexBuffer>(m_VertexBufferHandle.get());
	m_IndexBuffer = CreateRef<IndexBuffer>(m_IndexBufferHandle.get());

	m_IndicesCount = (uint32_t)indices.size();

}

void Render::Mesh::SetVertices(std::vector<Vertex>& vertices)
{
	this->vertices = vertices;
}

void Render::Mesh::SetIndices(std::vector<uint32_t>& indices)
{
	this->indices = indices;
}

void Render::Mesh::CalculateNormals()
{

	std::vector<Vertex> v;
	std::vector<uint32_t> in;

	for (int i = 0; i < this->indices.size(); i++) {
		v.push_back(vertices[indices[i]]);
		in.push_back(indices[i]);
		if (v.size() == 3) {
			Vertex& v1 = v[0];
			Vertex& v2 = v[1];
			Vertex& v3 = v[2];
			glm::vec3 pos1(v1.position.x, v1.position.y, v1.position.z);
			glm::vec3 pos2(v2.position.x, v2.position.y, v2.position.z);
			glm::vec3 pos3(v3.position.x, v3.position.y, v3.position.z);

			glm::vec3 A = pos2 - pos1;
			glm::vec3 B = pos3 - pos1;
			float Nx = A.y * B.z - A.z * B.y;
			float Ny = A.z * B.x - A.x * B.z;
			float Nz = A.x * B.y - A.y * B.x;

			glm::vec3 N(Nx, Ny, Nz);
			N = glm::normalize(N);
			// texture coordinates
			glm::vec2 uv1(v1.tx, v1.ty);
			glm::vec2 uv2(v2.tx, v2.ty);
			glm::vec2 uv3(v3.tx, v3.ty);

			glm::vec3 edge1 = pos2 - pos1;
			glm::vec3 edge2 = pos3 - pos1;
			glm::vec2 deltaUV1 = uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;

			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			v1.SetTangent(tangent);
			v2.SetTangent(tangent);
			v3.SetTangent(tangent);

			v1.SetNormal(N);
			v2.SetNormal(N);
			v3.SetNormal(N);

			vertices[in[0]] = v1;
			vertices[in[1]] = v2;
			vertices[in[2]] = v3;

			in.clear();
			v.clear();
		}
	}

}

bool Render::Mesh::Valid()
{
	return m_VertexBuffer && m_IndexBuffer;
}
