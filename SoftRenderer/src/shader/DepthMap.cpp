#include "DepthMap.h"

void DepthShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool DepthShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec3 worldPos = baryCentric.x * triangle.vertices[0].position + baryCentric.y * triangle.vertices[1].position + baryCentric.z * triangle.vertices[2].position;
	glm::vec4 clip = viewProjection * glm::vec4(worldPos, 1.0f);
	float ndcDepth = clip.z / clip.w;        // [-1, 1] (OpenGL convention: near -1, far +1)
	float gray = (ndcDepth + 1.0f) * 0.5f;   // remap to [0, 1]: near black, far white
	gl_FragColor = { gray, gray, gray, 1.0f };
	return false;
}