#pragma once

#include "znmsp.h"

#include "Buffer.h"

BEGIN_ENGINE

namespace Render
{
	/// <summary>
	/// Vertex buffer view, requires buffer type to be set to VERTEX_BUFFER
	/// </summary>
	class VertexBuffer
	{
	public:

		VertexBuffer() = default;
		VertexBuffer(Buffer* pBuffer);
		~VertexBuffer();

		Buffer* GetBuffer();

		nvrhi::BufferHandle Handle;

	private:
		Buffer* m_Buffer;
	};

}

END_ENGINE