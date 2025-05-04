#pragma once

#include "znmsp.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Vertex.h"

#include <vector>

BEGIN_ENGINE

namespace Render {

	enum class RenderMode {
		TRIANGLES,
		QUADS,
	};

	class Mesh {
	public:

		Mesh();
		~Mesh();
		Mesh(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		Mesh(const std::string& mfFile);

		void Update(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		void Update();
		void SetVertices(std::vector<Vertex>& vertices);
		void SetIndices(std::vector<uint32_t>& indices);

		void CalculateNormals();
		bool Valid();

		RenderMode renderMode = RenderMode::TRIANGLES;

		VertexBuffer* GetVertexBuffer() { return m_VertexBuffer.get(); }
		IndexBuffer* GetIndexBuffer() { return m_IndexBuffer.get(); }
		uint32_t GetIndicesCount() { return m_IndicesCount; }

		int32_t VertexBufferIndex = -1;
		int32_t IndexBufferIndex = -1;

		int32_t MaterialIndex = -1;
		glm::vec3 Min = { };
		glm::vec3 Max = { };
		glm::vec3 Centre = { };
		float_t Radius = 0.0f;
		bool IsRigged = false;

		int LodLevel = 0;

		size_t Hash;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

	private:

		Ref<Buffer> m_VertexBufferHandle;
		Ref<Buffer> m_IndexBufferHandle;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		uint32_t m_IndicesCount;

	};

}

END_ENGINE