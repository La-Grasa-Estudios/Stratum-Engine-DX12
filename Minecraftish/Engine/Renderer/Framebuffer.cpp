#include "Framebuffer.h"
#include "RendererContext.h"

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

	nvrhi::FramebufferDesc desc{};

	for (int i = 0; i < Desc.Attachments.size(); i++)
	{
		auto& att = Desc.Attachments[i];
		
		if (att.pImage->ImageDesc.IsDepthFormat())
		{
			desc.setDepthAttachment(att.pImage->Handle);
			continue;
		}

		desc.addColorAttachment(att.pImage->Handle);
	}

	Handle = RendererContext::GetDevice()->createFramebuffer(desc);

	lock.release();

}
