#pragma once

#include "znmsp.h"

#include "Buffer.h"

BEGIN_ENGINE

namespace Render
{

	/// <summary>
	/// Index buffer view, requires buffer type to be set to INDEX_BUFFER
	/// </summary>
	class IndexBuffer
	{
	public:

		IndexBuffer() = default;
		IndexBuffer(Buffer* pBuffer);
		~IndexBuffer();

		Buffer* GetBuffer();

		nvrhi::BufferHandle Handle;

	private:
		Buffer* m_Buffer;
		
	};

}

END_ENGINE