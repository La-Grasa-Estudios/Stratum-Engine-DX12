#include "Framebuffer.h"

using namespace ENGINE_NAMESPACE;

Render::Framebuffer::Framebuffer(FramebufferDesc desc)
{

	lock.acquire();

	m_IsWindowFb = desc.IsWindowSurfaceFb;

	if (m_IsWindowFb || desc.Attachments.empty()) return;

	m_Width = desc.Attachments[0].pImage->GetSize().x;
	m_Height = desc.Attachments[0].pImage->GetSize().y;

	Desc = desc;

	Create();

}

Render::Framebuffer::~Framebuffer()
{

}

void Render::Framebuffer::ClearAttachment(uint32_t attachmentIndex, float col[4])
{
	if (this->IsWindowFramebuffer()) return;
	glm::vec4 cc = glm::vec4(col[0], col[1], col[2], col[3]);
	
}

glm::ivec2 Render::Framebuffer::GetSize()
{
	return glm::ivec2(m_Width, m_Height);
}

bool Render::Framebuffer::IsWindowFramebuffer()
{
	return m_IsWindowFb;
}

void Render::Framebuffer::Create()
{

	if (Desc.IsWindowSurfaceFb) return;

	

	lock.release();

}
