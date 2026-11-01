#include "PbrShader2.h"
#define PI 3.14159265358979f

void PbrShader2::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, itModel * vertex.tangent, itModel * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool PbrShader2::Fragment(glm::vec4& gl_fragColor)
{
	glm::vec2 uv = baryCentric.x * triangle.vertices[0].uv + baryCentric.y * triangle.vertices[1].uv + baryCentric.z * triangle.vertices[2].uv;
	glm::vec3 worldPos = baryCentric.x * triangle.vertices[0].position + baryCentric.y * triangle.vertices[1].position + baryCentric.z * triangle.vertices[2].position;
	glm::vec3 worldNormal = glm::normalize(baryCentric.x * triangle.vertices[0].normal + baryCentric.y * triangle.vertices[1].normal + baryCentric.z * triangle.vertices[2].normal);

	//构造TBN:插值切线/副切线 + 插值法线 = 原始 Darboux 帧(与参考实现 D={t,b,n} 一致)
	glm::vec3 N = worldNormal;
	glm::vec3 T = glm::normalize(baryCentric.x * triangle.vertices[0].tangent + baryCentric.y * triangle.vertices[1].tangent + baryCentric.z * triangle.vertices[2].tangent);
	glm::vec3 B = glm::normalize(baryCentric.x * triangle.vertices[0].bitangent + baryCentric.y * triangle.vertices[1].bitangent + baryCentric.z * triangle.vertices[2].bitangent);
	glm::mat3 TBN = {
		T, B, N
	};
	glm::vec3 samplerN = glm::normalize(Sampler2D(uv, tbnNormalTexture)) * 2.0f - 1.0f;
	glm::vec3 NORMAL = glm::normalize(TBN * samplerN);

	//光照方向取反，从像素出发

	glm::vec3 V = glm::normalize(camPos - worldPos);
	glm::vec3  R = glm::reflect(-V, NORMAL);
	glm::vec3 lo{ 0.0f };
	glm::vec3 F0{ 0.04f };
	float ao = Sampler2D(uv, aoTexture).r;
	float roughness = Sampler2D(uv, roughnessTexture).r;
	float metalness = Sampler2D(uv, metalnessTexture).r;

	glm::vec3 albedo = Sampler2D(uv, diffuseTexture);

	F0 = glm::mix(F0, albedo, metalness);

	for (uint32_t i = 0; i < pointLights.size(); i++)
	{
		const std::shared_ptr<PointLight>& plight = pointLights[i];
		glm::vec3 L = glm::normalize(plight->GetPosition() - worldPos);
		glm::vec3 lightColor = plight->GetColor();
		float distance = glm::distance(plight->GetPosition(), worldPos);
		glm::vec3 H = glm::normalize(V + L);
		lightColor = 1.0f / (distance * distance) * lightColor;
		float NDF = DistributionGGX(NORMAL, H, roughness);
		float G = GeometrySmith(NORMAL, V, L, roughness);
		glm::vec3 F = FreshelSchlick(glm::clamp(glm::dot(H, V), 0.0f, 1.0f), F0);

		glm::vec3 numerator = NDF * F * G;
		float denominator = 4.0f * glm::max(glm::dot(NORMAL, V), 0.0f) * glm::max(glm::dot(NORMAL, L), 0.0f) + 0.0001;
		glm::vec3 specBRDF = numerator / denominator;
		glm::vec3 Ks = F;
		glm::vec3 Kd = glm::vec3(1.0f) - Ks;
		Kd *= (1.0f - metalness);
		float NdotL = glm::max(glm::dot(NORMAL, L), 0.0f);
		lo += (Kd * albedo / PI + specBRDF) * lightColor * NdotL;
	}

	glm::vec3 ambient = glm::vec3(0.03f) * albedo * ao;
	glm::vec3 color = ambient + lo;

	color *= 0.6f;
	color = glm::max(glm::vec3(0.0f), color - 0.004f);
	color = glm::pow((color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f), glm::vec3(2.2));

	//伽马矫正
	color = glm::pow(color, glm::vec3(1.0f / 2.2f));
	gl_fragColor = { color, 1.0f };
	return false;
}

float PbrShader2::DistributionGGX(const glm::vec3& N, const glm::vec3& H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = glm::max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return nom / denom;
}

float PbrShader2::GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float PbrShader2::GeometrySmith(const glm::vec3& N, const glm::vec3& V, const glm::vec3& L, float roughness)
{
	float NdotV = glm::max(glm::dot(N, V), 0.0f);
	float NdotL = glm::max(glm::dot(N, L), 0.0f);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

glm::vec3 PbrShader2::FreshelSchlick(float cosTheta, const glm::vec3& F0)
{
	return F0 + (1.0f - F0) * glm::pow(glm::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}