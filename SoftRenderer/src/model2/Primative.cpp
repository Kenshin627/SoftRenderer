#include "Primative.h"

Primative::Primative(size_t nface):nface(nface)
{
	positions.resize(nface * 3);
	normals.resize(nface * 3);
	uvs.resize(nface * 3);
}

const glm::vec3& Primative::GetPosition(size_t nface, size_t triangleVertexIndex) const
{
	return positions[nface * 3 + triangleVertexIndex];
}

const glm::vec3& Primative::GetNormal(size_t nface, size_t triangleVertexIndex) const
{
	return normals[nface * 3 + triangleVertexIndex];
}

const glm::vec2& Primative::GetUV(size_t nface, size_t triangleVertexIndex) const
{
	return uvs[nface * 3 + triangleVertexIndex];
}

void Primative::InerstPosition(size_t idx, float x, float y, float z)
{
	positions[idx] = { x, y, z };
}

void Primative::InsertNormal(size_t idx, float x, float y, float z)
{
	normals[idx] = glm::normalize(glm::vec3(x, y, z));
}

void Primative::InsertUV(size_t idx, float u, float v)
{
	uvs[idx] = { u, v };
}