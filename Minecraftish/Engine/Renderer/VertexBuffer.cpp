#include "VertexBuffer.h"

#include "DX12/GlobalSharedData.h"

using namespace ENGINE_NAMESPACE;

Render::VertexBuffer::VertexBuffer(Buffer* pBuffer)
{

	m_Buffer = pBuffer;
	this->Handle = pBuffer->Handle;

}

Render::VertexBuffer::~VertexBuffer()
{
	
}

Render::Buffer* Render::VertexBuffer::GetBuffer()
{
	return this->m_Buffer;
}
