#include "Vertex.h"

#include <glm/gtc/packing.hpp>
#include <glm/detail/type_half.hpp>

using namespace ENGINE_NAMESPACE;

void Render::Vertex::SetPosition(float x, float y, float z)
{
    position = glm::vec3(x, y, z);
    
}

void Render::Vertex::SetUV(glm::vec2& v)
{
    SetUV(v.x, v.y);
}

void Render::Vertex::SetNormal(glm::vec3& v)
{
    SetNormal(v.x, v.y, v.z);
}

void Render::Vertex::SetTangent(glm::vec3& v)
{
    SetTangent(v.x, v.y, v.z);
}

void Render::Vertex::SetPosition(glm::vec3& v)
{
    SetPosition(v.x, v.y, v.z);
}

Render::Vertex::Vertex(float x, float y, float z, float u, float v) : tx(u), ty(v)
{
    position = { x, y, z };
    normal = {};
    tangent = {};
}

Render::Vertex::Vertex(float x, float y, float z, float u, float v, float nx_, float ny_, float nz_) : tx(u), ty(v) {
    position = { x, y, z };
    normal = { nx_, ny_, nz_, 0.0f };
    tangent = {};
}

Render::Vertex::Vertex(float x, float y, float z)
{
    position = { x, y, z };
    tx = 0.0f, ty = 0.0f;
    normal = {};
    tangent = {};
}

void Render::Vertex::SetUV(float x, float y)
{
    tx = x; ty = y;
}

void Render::Vertex::SetNormal(float x, float y, float z)
{
    normal = { x, y, z, 0.0f };
}

void Render::Vertex::SetTangent(float x, float y, float z)
{
    tangent = { x, y, z, 0.0f };
}

ENGINE_NAMESPACE::Render::ShaderVertexLayout Render::Vertex::GetLayout()
{

    ShaderVertexLayout layout;
    
    VertexAttribute position{};
    position.Index = 0;
    position.InputRate = VertexInputRate::PER_VERTEX;
    position.Type = VertexType::FLOAT3_32;
    position.Offset = offsetof(Render::Vertex, position);

    VertexAttribute texCoord{};
    texCoord.Index = 1;
    texCoord.InputRate = VertexInputRate::PER_VERTEX;
    texCoord.Type = VertexType::FLOAT2_16;
    texCoord.Offset = offsetof(Render::Vertex, tx);

    VertexAttribute normal{};
    normal.Index = 2;
    normal.InputRate = VertexInputRate::PER_VERTEX;
    normal.Type = VertexType::INT4_8_NORM;
    normal.Offset = offsetof(Render::Vertex, normal);

    VertexAttribute tangent{};
    tangent.Index = 3;
    tangent.InputRate = VertexInputRate::PER_VERTEX;
    tangent.Type = VertexType::INT4_8_NORM;
    tangent.Offset = offsetof(Render::Vertex, tangent);

    VertexAttribute boneIds{};
    boneIds.Index = 4;
    boneIds.InputRate = VertexInputRate::PER_VERTEX;
    boneIds.Type = VertexType::UBYTE4;
    boneIds.Offset = offsetof(Render::Vertex, BoneIds);

    VertexAttribute boneWeights{};
    boneWeights.Index = 5;
    boneWeights.InputRate = VertexInputRate::PER_VERTEX;
    boneWeights.Type = VertexType::INT4_16_NORM;
    boneWeights.Offset = offsetof(Render::Vertex, BoneWeights);

    layout.VertexAttributes.push_back(position);
    layout.VertexAttributes.push_back(texCoord);
    layout.VertexAttributes.push_back(normal);
    layout.VertexAttributes.push_back(tangent);
    layout.VertexAttributes.push_back(boneIds);
    layout.VertexAttributes.push_back(boneWeights);

    layout.Stride = sizeof(Vertex);

    return layout;
}
