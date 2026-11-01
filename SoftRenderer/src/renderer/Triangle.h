#pragma once
#include <glm/glm.hpp>
#include <array>

struct VertexAttribute
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	// tangent frame of the face: (t b) = U^-1 * E (tutorial tangent chapter);
	// carried through the pipeline as varyings and interpolated per fragment
	// like the normal, combined with the smooth normal into the Darboux frame
	glm::vec3 tangent;
	glm::vec3 bitangent;
};

struct Triangle
{
	std::array<VertexAttribute, 3> vertices;
};