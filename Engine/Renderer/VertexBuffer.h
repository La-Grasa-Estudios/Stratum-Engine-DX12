#pragma once

#include "znmsp.h"

#include "Buffer.h"

BEGIN_ENGINE

namespace Render
{

	class VertexBuffer
	{
	public:

		VertexBuffer(Buffer* pBuffer);
		~VertexBuffer();

		Buffer* GetBuffer();

		template<typename T>
		T* As()
		{
			return (T*)NativeData;
		}

		void* NativeData;

	private:
		Buffer* m_Buffer;
	};

}

END_ENGINE