#include "Primative.h"

Primative::Primative(size_t nface):nface(nface)
{
	positions.resize(nface * 3);
	normals.resize(nface * 3);
	uvs.resize(nface * 3, glm::vec2(0.0f));
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
	glm::vec3 n(x, y, z);
	normals[idx] = glm::dot(n, n) > 0.0f ? glm::normalize(n) : glm::vec3(0, 0, 1);
}

void Primative::InsertUV(size_t idx, float u, float v)
{
	uvs[idx] = { u, v };
}