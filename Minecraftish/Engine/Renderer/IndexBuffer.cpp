#include "IndexBuffer.h"

using namespace ENGINE_NAMESPACE;

Render::IndexBuffer::IndexBuffer(Buffer* pBuffer)
{

	m_Buffer = pBuffer;

	this->Handle = pBuffer->Handle;

}

Render::IndexBuffer::~IndexBuffer()
{
	
}

Render::Buffer* Render::IndexBuffer::GetBuffer()
{
	return this->m_Buffer;
}
