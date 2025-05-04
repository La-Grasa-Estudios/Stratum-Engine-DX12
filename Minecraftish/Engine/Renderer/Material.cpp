#include "Material.h"

using namespace ENGINE_NAMESPACE;

Render::Material::Material()
{
	Diffuse   = {};
	Normal    = {};
	Roughness = {};
	Metalness = {};
	Specular  = {};
}

Render::Material::~Material()
{
	//std::cout << "Deleting material " << Diffuse.PathToTextureFile << std::endl;
}

void Render::Material::ComputeHash()
{
	uintptr_t A = reinterpret_cast<uintptr_t>(Diffuse.BaseImage.get());
	uintptr_t B = reinterpret_cast<uintptr_t>(Roughness.BaseImage.get());
	uintptr_t C = reinterpret_cast<uintptr_t>(Metalness.BaseImage.get());
	uintptr_t D = reinterpret_cast<uintptr_t>(Normal.BaseImage.get());

	size_t VA = std::hash<glm::vec4>::_Do_hash(this->albedo);
	size_t VB = std::hash<glm::vec4>::_Do_hash(this->m_r_a);

	size_t BLEND = std::hash<int>()((int)BlendMode);

	size_t TEX = A ^ B ^ C ^ D;
	Hash = VA ^ VB ^ BLEND ^ TEX;
}	

Render::MaterialTexture::MaterialTexture()
{
	PathToTextureFile = "";
}

Render::MaterialTexture::MaterialTexture(Ref<ImageResource> s, bool t)
{
	PathToTextureFile = "";
	BaseImage = s;
	IsSamplerTransparent = t;
}

Render::ImageResource* Render::MaterialTexture::GetTexture()
{
	return HqImage ? HqImage.get() : BaseImage.get();
}
