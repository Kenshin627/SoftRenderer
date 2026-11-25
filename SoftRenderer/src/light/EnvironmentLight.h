#pragma once
#include <glm/glm.hpp>
#include <array>
#include <vector>

class EnvironmentLight
{
public:
	explicit EnvironmentLight(const char* path);
	void SetRotation(float degrees);
	float intensity = 1.0f;

	glm::vec3 Evaluate(const glm::vec3& normal, const glm::vec3& view, const glm::vec3& albedo,
	                   const glm::vec3& f0, float roughness, float metallic) const;
private:
	struct Map
	{
		int width = 0, height = 0;
		std::vector<glm::vec3> pixels;

		Map() = default;
		Map(int w, int h) : width(w), height(h), pixels(size_t(w) * h) {}
		glm::vec3 Sample(glm::vec2 uv) const;
		Map Reduce() const;
	};

	static Map ConvolveDiffuse(const Map& source);
	static Map Prefilter(const std::vector<Map>& source, int width, float roughness);
	Map irradianceOverPi;
	std::array<Map, 6> reflection;
	float longitudeOffset = 0.0f;
};
