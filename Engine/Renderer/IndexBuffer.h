#pragma once

#include "znmsp.h"

#include "Buffer.h"

BEGIN_ENGINE

namespace Render
{

	class IndexBuffer
	{
	public:

		IndexBuffer(Buffer* pBuffer);
		~IndexBuffer();

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