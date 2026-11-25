#include "EnvironmentLight.h"
#include <stbimage/stb_image.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	constexpr float pi = 3.14159265358979323846f;
	constexpr int sampleCount = 256;
	constexpr int lutSize = 64;

	glm::vec2 DirectionUv(const glm::vec3& direction)
	{
		return {0.5f + std::atan2(direction.z, direction.x) / (2.0f * pi),
		        std::acos(glm::clamp(direction.y, -1.0f, 1.0f)) / pi};
	}

	glm::vec3 DirectionAt(int x, int y, int width, int height)
	{
		float phi = 2.0f * pi * ((x + 0.5f) / width - 0.5f);
		float theta = pi * (y + 0.5f) / height;
		return {std::cos(phi) * std::sin(theta), std::cos(theta), std::sin(phi) * std::sin(theta)};
	}

	glm::vec3 GgxHalfVector(int i, float alphaSquared)
	{
		float phi = 2.0f * pi * float(i) / sampleCount;
		// Hammersley sample on [0, 1)^2, mapped to the GGX half-vector distribution.
		float u = float(glm::bitfieldReverse(uint32_t(i))) * 2.3283064365386963e-10f;
		float cosTheta = std::sqrt((1.0f - u) / (1.0f - u + alphaSquared * u));
		float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		return {std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta};
	}

	float SmithG1(float cosine, float alphaSquared)
	{
		return 2.0f * cosine / (cosine + std::sqrt(alphaSquared + (1.0f - alphaSquared) * cosine * cosine));
	}

	using BrdfTable = std::array<glm::vec2, lutSize * lutSize>;

	BrdfTable IntegrateBrdf()
	{
		BrdfTable table{};
		for (int y = 0; y < lutSize; ++y)
		{
			float roughness = float(y) / (lutSize - 1);
			float a2 = roughness * roughness * roughness * roughness;
			std::array<glm::vec3, sampleCount> halfVectors;
			for (int i = 0; i < sampleCount; ++i) halfVectors[i] = GgxHalfVector(i, a2);
			for (int x = 0; x < lutSize; ++x)
			{
				float nv = glm::max(0.0001f, float(x) / (lutSize - 1));
				glm::vec3 v(std::sqrt(1.0f - nv * nv), 0.0f, nv);
				glm::vec2 sum(0.0f);
				float gv = SmithG1(nv, a2);
				for (const auto& h : halfVectors)
				{
					float vh = glm::max(0.0f, glm::dot(v, h));
					float nl = 2.0f * vh * h.z - nv;
					if (nl <= 0.0f) continue;
					float visibility = gv * SmithG1(nl, a2) * vh / (h.z * nv);
					float fc = std::pow(1.0f - glm::min(vh, 1.0f), 5.0f);
					sum += visibility * glm::vec2(1.0f - fc, fc);
				}
				table[y * lutSize + x] = sum / float(sampleCount);
			}
		}
		return table;
	}

	glm::vec2 SampleBrdf(float nv, float roughness)
	{
		static const BrdfTable table = IntegrateBrdf();
		glm::vec2 pos = glm::vec2(nv, roughness) * float(lutSize - 1);
		glm::ivec2 lo(pos), hi = glm::min(lo + 1, glm::ivec2(lutSize - 1));
		glm::vec2 f = glm::fract(pos);
		return glm::mix(glm::mix(table[lo.y * lutSize + lo.x], table[lo.y * lutSize + hi.x], f.x),
		                glm::mix(table[hi.y * lutSize + lo.x], table[hi.y * lutSize + hi.x], f.x), f.y);
	}
}

glm::vec3 EnvironmentLight::Map::Sample(glm::vec2 uv) const
{
	float x = glm::fract(uv.x) * width - 0.5f;
	float y = glm::clamp(uv.y * height - 0.5f, 0.0f, float(height - 1));
	int ix = int(std::floor(x)), iy = int(y);
	float fx = x - ix, fy = y - iy;
	int x0 = ix < 0 ? width - 1 : ix, x1 = (x0 + 1) % width;
	const auto* row0 = pixels.data() + size_t(iy) * width;
	const auto* row1 = pixels.data() + size_t(glm::min(iy + 1, height - 1)) * width;
	return glm::mix(glm::mix(row0[x0], row0[x1], fx), glm::mix(row1[x0], row1[x1], fx), fy);
}

EnvironmentLight::Map EnvironmentLight::Map::Reduce() const
{
	Map result(glm::max(1, width / 2), glm::max(1, height / 2));
	// Cosine differences integrate latitude bands exactly, including partial rows in odd-sized mips.
	for (int y = 0; y < result.height; ++y)
	{
		double top = double(y) * height / result.height, bottom = double(y + 1) * height / result.height;
		for (int x = 0; x < result.width; ++x)
		{
			double left = double(x) * width / result.width, right = double(x + 1) * width / result.width;
			glm::dvec3 sum(0.0);
			double weight = 0.0;
			for (int sy = int(top); sy < bottom; ++sy)
			{
				double band = std::cos(double(pi) * glm::max(top, double(sy)) / height)
				            - std::cos(double(pi) * glm::min(bottom, double(sy + 1)) / height);
				for (int sx = int(left); sx < right; ++sx)
				{
					double area = band * (glm::min(right, double(sx + 1)) - glm::max(left, double(sx)));
					sum += glm::dvec3(pixels[size_t(sy) * width + sx]) * area;
					weight += area;
				}
			}
			result.pixels[size_t(y) * result.width + x] = glm::vec3(sum / weight);
		}
	}
	return result;
}

EnvironmentLight::Map EnvironmentLight::ConvolveDiffuse(const Map& source)
{
	constexpr int w = 128, h = 64;
	struct Patch { glm::vec3 direction, radiance; float area; };
	std::vector<Patch> sky;
	sky.reserve(w * h);
	for (int y = 0; y < h; ++y)
	{
		float area = std::cos(pi * y / h) - std::cos(pi * (y + 1) / h);
		for (int x = 0; x < w; ++x)
			sky.push_back({DirectionAt(x, y, w, h), source.Sample({(x + 0.5f) / w, (y + 0.5f) / h}), area});
	}
	Map result(64, 32);
	for (int y = 0; y < result.height; ++y)
		for (int x = 0; x < result.width; ++x)
		{
			glm::vec3 n = DirectionAt(x, y, result.width, result.height);
			glm::vec3 sum(0.0f);
			float weight = 0.0f;
			for (const auto& patch : sky)
			{
				float wCos = patch.area * glm::max(0.0f, glm::dot(patch.direction, n));
				sum += patch.radiance * wCos;
				weight += wCos;
			}
			result.pixels[y * result.width + x] = sum / weight;
		}
	return result;
}

EnvironmentLight::Map EnvironmentLight::Prefilter(const std::vector<Map>& source, int width, float roughness)
{
	struct Tap { glm::vec3 direction; float solidAngle; };
	std::vector<Tap> taps;
	float a2 = roughness * roughness * roughness * roughness;
	float weight = 0.0f;
	for (int i = 0; i < sampleCount; ++i)
	{
		glm::vec3 h = GgxHalfVector(i, a2);
		glm::vec3 l = 2.0f * h.z * h - glm::vec3(0, 0, 1);
		if (l.z <= 0.0f) continue;
		float denom = (1.0f - h.z * h.z) + a2 * h.z * h.z;
		float pdf = a2 / (4.0f * pi * denom * denom); // V = N in the split-sum prefilter.
		taps.push_back({l, 1.0f / (sampleCount * pdf)});
		weight += l.z;
	}

	Map result(width, glm::max(1, width / 2));
	float pixelAngle = 2.0f * pi * pi / (float(source[0].width) * source[0].height);
	float polarBand = std::sin(0.5f * pi / source[0].height);
	for (int y = 0; y < result.height; ++y)
		for (int x = 0; x < result.width; ++x)
		{
			glm::vec3 n = DirectionAt(x, y, result.width, result.height);
			glm::vec3 tangent = glm::normalize(glm::cross(glm::vec3(0, 1, 0), n));
			glm::vec3 bitangent = glm::cross(n, tangent);
			glm::vec3 sum(0.0f);
			for (const auto& tap : taps)
			{
				const auto& local = tap.direction;
				glm::vec3 l = tangent * local.x + bitangent * local.y + n * local.z;
				glm::vec2 uv = DirectionUv(l);
				float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - l.y * l.y));
				float texelAngle = pixelAngle * glm::max(polarBand, sinTheta);
				float lod = glm::clamp(0.5f * std::log2(tap.solidAngle / texelAngle), 0.0f, float(source.size() - 1));
				size_t lo = size_t(lod), hi = glm::min(lo + 1, source.size() - 1);
				sum += local.z * glm::mix(source[lo].Sample(uv), source[hi].Sample(uv), lod - float(lo));
			}
			result.pixels[y * result.width + x] = sum / weight;
		}
	return result;
}

EnvironmentLight::EnvironmentLight(const char* path)
{
	int width, height, channels;
	if (!stbi_is_hdr(path) || !stbi_info(path, &width, &height, &channels) || height <= 0 || width != 2LL * height)
		throw std::runtime_error(std::string("expected a 2:1 Radiance HDR environment: ") + path);
	std::unique_ptr<float, decltype(&stbi_image_free)> rgb(stbi_loadf(path, &width, &height, &channels, 3), stbi_image_free);
	if (!rgb) throw std::runtime_error(std::string("could not read HDR environment: ") + path);
	std::vector<Map> source;
	source.emplace_back(width, height);
	for (size_t i = 0; i < source.front().pixels.size(); ++i)
		source.front().pixels[i] = {rgb.get()[3 * i], rgb.get()[3 * i + 1], rgb.get()[3 * i + 2]};
	rgb.reset();
	while (source.back().width > 1 || source.back().height > 1)
		source.push_back(source.back().Reduce());
	size_t diffuseLevel = 0;
	while (source[diffuseLevel].width > 128) ++diffuseLevel;
	irradianceOverPi = ConvolveDiffuse(source[diffuseLevel]);
	for (size_t i = 1; i < reflection.size(); ++i)
	{
		int levelWidth = glm::min(width, 256 >> (i - 1));
		reflection[i] = Prefilter(source, levelWidth, float(i) / float(reflection.size() - 1));
	}
	reflection[0] = std::move(source[0]);
}

void EnvironmentLight::SetRotation(float degrees)
{
	// Inverse rotation of the lookup direction for a positive right-handed rotation about +Y.
	longitudeOffset = std::remainder(degrees, 360.0f) / 360.0f;
}

glm::vec3 EnvironmentLight::Evaluate(const glm::vec3& normal, const glm::vec3& view, const glm::vec3& albedo,
                                     const glm::vec3& f0, float roughness, float metallic) const
{
	float nv = glm::dot(normal, view);
	if (!std::isfinite(nv)) return glm::vec3(0.0f);
	nv = glm::clamp(nv, 0.0f, 1.0f);
	glm::vec2 offset(longitudeOffset, 0.0f);
	glm::vec3 diffuse = irradianceOverPi.Sample(DirectionUv(normal) + offset);
	glm::vec2 uv = DirectionUv(glm::reflect(-view, normal)) + offset;
	float level = roughness * float(reflection.size() - 1);
	size_t lo = size_t(level), hi = glm::min(lo + 1, reflection.size() - 1);
	glm::vec3 radiance = glm::mix(reflection[lo].Sample(uv), reflection[hi].Sample(uv), level - float(lo));
	glm::vec2 brdf = SampleBrdf(nv, roughness);
	glm::vec3 fresnel = f0 + (glm::max(glm::vec3(1.0f - roughness), f0) - f0) * std::pow(1.0f - nv, 5.0f);
	glm::vec3 kd = (1.0f - fresnel) * (1.0f - metallic);
	return intensity * (kd * albedo * diffuse + radiance * (f0 * brdf.x + brdf.y));
}
