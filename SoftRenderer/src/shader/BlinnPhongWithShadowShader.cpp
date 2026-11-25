#include "BlinnPhongWithShadowShader.h"

// NDC 深度偏移:光空间正交 [-1,1] 跨 10 个世界单位,5e-3 约 2.5cm,足够覆盖插值误差又不产生漏光
constexpr float SHADOW_BIAS = 5e-3f;

void BlinnPhongWithShadowShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, glm::mat3(model) * vertex.tangent, glm::mat3(model) * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1.0);
	if (vertexIndex == 2 && shadowBuffer && shadowFilter == ShadowFilter::PCF)
	{
		// Directional projection is affine. Fit receiver depth in shadow texel coordinates.
		const auto& v = triangle.vertices;
		glm::mat3 transform(worldToShadow);
		glm::vec3 plane = glm::cross(transform * (v[1].position - v[0].position),
		                             transform * (v[2].position - v[0].position));
		shadowDepthGradient = glm::abs(plane.z) > 1e-8f ? -glm::vec2(plane) / plane.z : glm::vec2(0.0f);
	}
}

float BlinnPhongWithShadowShader::ShadowFactor(const glm::vec3& worldPos) const
{
	if (!shadowBuffer) return 1.0f;
	glm::vec4 projected = worldToShadow * glm::vec4(worldPos, 1.0f);
	glm::vec3 p = glm::vec3(projected) / projected.w;
	int width = static_cast<int>(shadowBuffer->width);
	int height = static_cast<int>(shadowBuffer->height);
	if (!(p.z >= -1.0f && p.z <= 1.0f && p.x >= 0.0f && p.x < width && p.y >= 0.0f && p.y < height))
		return 1.0f;

	int x = static_cast<int>(p.x), y = static_cast<int>(p.y);
	constexpr float shadowLight = 0.3f;
	if (shadowFilter == ShadowFilter::Hard)
		return shadowBuffer->zBuffer[y * width + x] + SHADOW_BIAS < p.z ? shadowLight : 1.0f;

	int blocked = 0;
	for (int dy = -1; dy <= 1; ++dy)
	{
		int sy = glm::clamp(y + dy, 0, height - 1);
		for (int dx = -1; dx <= 1; ++dx)
		{
			int sx = glm::clamp(x + dx, 0, width - 1);
			glm::vec2 offset(sx + 0.5f - p.x, sy + 0.5f - p.y);
			float receiverDepth = p.z + glm::dot(shadowDepthGradient, offset);
			if (shadowBuffer->zBuffer[sy * width + sx] + SHADOW_BIAS < receiverDepth)
				++blocked;
		}
	}
	return shadowLight + (1.0f - shadowLight) * ((9 - blocked) / 9.0f);
}

bool BlinnPhongWithShadowShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec2 uv = baryCentric.x * triangle.vertices[0].uv + baryCentric.y * triangle.vertices[1].uv + baryCentric.z * triangle.vertices[2].uv;
	glm::vec3 worldPos = baryCentric.x * triangle.vertices[0].position + baryCentric.y * triangle.vertices[1].position + baryCentric.z * triangle.vertices[2].position;
	float shadow = ShadowFactor(worldPos);

	glm::vec3 normal = SurfaceNormal(uv);

	//光照方向取反，从像素出发
	glm::vec3 lightDir = glm::normalize(-light->Direction());
	glm::vec3 lightColor = light->Color();

	//diffuse
	glm::vec3 diffuseColor = Sampler2D(uv, diffuseTexture); //[0-1]
	float diffuse = glm::max(0.0f, glm::dot(lightDir, normal));
	diffuseColor = diffuse * diffuseColor;

	//specular
	glm::vec3 viewDir = glm::normalize(camPos - worldPos);
	glm::vec3 h = glm::normalize((viewDir + lightDir));
	float alignment = reflectionPhong ? glm::dot(viewDir, glm::reflect(-lightDir, normal)) : glm::dot(h, normal);
	float specular = diffuse > 0.0f ? glm::pow(glm::max(0.0f, alignment), reflectionPhong ? 32.0f : 128.0f) : 0.0f;
	float samplerSpecular = Sampler2D(uv, specularTexture).b;//[0-1]
	glm::vec3 specularColor = glm::vec3(samplerSpecular);
	specularColor = specularColor * specular;

	//ambient 0.4 matches the reference PhongShader; it is multiplied by the albedo
	//(reference: color * (ambient + diffuse + specular)), keeping shadowed sides
	//readable without the flat white wash of an unmodulated ambient
	glm::vec3 ambientColor = 0.4f * Sampler2D(uv, diffuseTexture);

	//emissive: _glow.tga self-illumination, unaffected by shadow/ao/light color (tutorial
	//textures chapter mentions the _glow.tga incandescence maps); null falls back to black
	//unlike albedo samplers (white fallback would white out untextured scenes)
	glm::vec3 emissiveColor = emissiveTexture ? Sampler2D(uv, emissiveTexture) : glm::vec3(0.0f);

	glm::vec3 color = ((diffuseColor + specularColor) * shadow + ambientColor * aoFactor) * lightColor + emissiveColor;
	color.r = glm::min(1.0f, color.r);
	color.g = glm::min(1.0f, color.g);
	color.b = glm::min(1.0f, color.b);
	gl_FragColor = { color, 1.0f };
	return false;
}