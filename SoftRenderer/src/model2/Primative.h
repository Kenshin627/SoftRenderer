#pragma once
#include "../material/Material.h"
#include<glm/glm.hpp>
#include <vector>
#include <memory>

class Primative
{
public:
	Primative(size_t nface);
	~Primative() = default;
	const glm::vec3& GetPosition(size_t nface, size_t triangleVertexIndex) const;
	void InerstPosition(size_t idx, float x, float y, float z);
	void InsertNormal(size_t idx, float x, float y, float z);
	void InsertUV(size_t idx, float u, float v);
	void SetMaterial(const std::shared_ptr<Material>& mat) { material = mat; }
	const std::shared_ptr<Material>& GetMaterial() const { return material; }
	const glm::vec3& GetNormal(size_t nface, size_t triangleVertexIndex) const;
	const glm::vec2& GetUV(size_t nface, size_t triangleVertexIndex) const;
	const size_t getNface() const { return nface; }
private:
	size_t nface;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::shared_ptr<Material> material;
};