#include "TBNShader.h"
#include <glm/gtc/matrix_transform.hpp>

void TBNShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	positions[gl_VertexIndex] = modelMatrix * vertex.position;
	normals[gl_VertexIndex] = glm::normalize(invertTransposeModelMatrix * vertex.normal);
	uvs[gl_VertexIndex] = vertex.uv;
	gl_Position = viewprojectionMatrix * modelMatrix * vertex.position;
}

//return discard
bool TBNShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec3 lightDir = glm::normalize(-dLight.Direction());
	glm::vec3 lightColor = dLight.Color();
	glm::vec2 uv = uvs[0] * baryCentricCoords.x + uvs[1] * baryCentricCoords.y + uvs[2] * baryCentricCoords.z;
	glm::vec3 pos = positions[0] * baryCentricCoords.x + positions[1] * baryCentricCoords.y + positions[2] * baryCentricCoords[2];

	//º∆À„TBNæÿ’Û
	glm::vec3 N = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	N = glm::normalize(N);
	glm::vec3 n = Sampler2D(uv, normalMap);
	n = { n.x * 2.0 - 1.0, n.y * 2.0 - 1.0, n.z * 2.0 - 1.0 };
	tangent = glm::normalize(glm::cross(bitangent, N));
	bitangent = glm::normalize(glm::cross(N, tangent));
	glm::mat3 TBN = { tangent, bitangent, N };
	n = TBN * n;
	n = glm::normalize(n);
	glm::vec3 h = glm::normalize((lightDir + glm::normalize(cameraPos - pos)));
	float diffuseCoeff = glm::max<float>(0.0, glm::dot(n, lightDir));
	glm::vec3 diffuseColor = Sampler2D(uv, diffuseTexture);	
	gl_FragColor = { diffuseCoeff * diffuseColor.r * lightColor.r, diffuseCoeff * diffuseColor.g * lightColor.g, diffuseCoeff * diffuseColor.b * lightColor.b, 255.0 };
	//gl_FragColor = { c.r, c.g, c.b, 255 };
	return false;
}