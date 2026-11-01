#include "NormalMap.h"

void NormalMapShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, itModel * vertex.tangent, itModel * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool NormalMapShader::Fragment(glm::vec4& gl_FragColor)
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
	glm::vec3 normal = glm::normalize(TBN * samplerN);

	gl_FragColor = { glm::normalize(worldNormal * 2.0f - 1.0f), 1.0f};
	return false;
}