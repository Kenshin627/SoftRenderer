#include "FlatShader.h"

void FlatShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0f);
}

bool FlatShader::Fragment(glm::vec4& gl_FragColor)
{
	// flat shading: one constant face normal per triangle (tutorial shading chapter starting point)
	glm::vec3 e1 = triangle.vertices[1].position - triangle.vertices[0].position;
	glm::vec3 e2 = triangle.vertices[2].position - triangle.vertices[0].position;
	glm::vec3 faceNormal = glm::normalize(glm::cross(e1, e2));
	glm::vec3 lightDir = glm::normalize(-light->Direction());
	float intensity = glm::max(0.0f, glm::dot(lightDir, faceNormal));
	gl_FragColor = { baseColor * intensity, 1.0f };
	return false;
}