#include "NormalMap.h"

void NormalMapShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, glm::mat3(model) * vertex.tangent, glm::mat3(model) * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool NormalMapShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec2 uv = baryCentric.x * triangle.vertices[0].uv + baryCentric.y * triangle.vertices[1].uv + baryCentric.z * triangle.vertices[2].uv;
	glm::vec3 normal = SurfaceNormal(uv);

	gl_FragColor = { normal * 0.5f + 0.5f, 1.0f};
	return false;
}
bool MaterialPreviewShader::Fragment(glm::vec4& color)
{
	const auto& v = triangle.vertices;
	glm::vec2 uv = baryCentric.x * v[0].uv + baryCentric.y * v[1].uv + baryCentric.z * v[2].uv;
	glm::vec3 rgb = showEmission && !emissiveTexture ? glm::vec3(0.0f)
		: Sampler2D(uv, showEmission ? emissiveTexture : diffuseTexture);
	color = glm::vec4(rgb, 1.0f);
	return false;
}
