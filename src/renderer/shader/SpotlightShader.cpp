#include "SpotlightShader.h"
#include <glm/gtc/matrix_transform.hpp>

void SpotlightShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	positions[gl_VertexIndex] = modelMatrix * vertex.position;
	normals[gl_VertexIndex] = glm::normalize(invertTransposeModelMatrix * vertex.normal);
	uvs[gl_VertexIndex] = vertex.uv;
	gl_Position = viewprojectionMatrix * modelMatrix * vertex.position;
}

//return discard
bool SpotlightShader::Fragment(glm::vec4& gl_FragColor)
{
	float ambient = 0.08;
	//重心坐标插值计算当前像素点法线，逐像素计算光照
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	glm::vec2 uv = uvs[0] * baryCentricCoords.x + uvs[1] * baryCentricCoords.y + uvs[2] * baryCentricCoords.z;
	glm::vec3 pos = positions[0] * baryCentricCoords.x + positions[1] * baryCentricCoords.y + positions[2] * baryCentricCoords[2];
	n = glm::normalize(n);
	glm::vec3 lightDir = glm::normalize(-sLight.Direction());

	float diffuseCoeff = glm::max<float>(0.0, glm::dot(n, lightDir));
	glm::vec3 diffuseColor = Sampler2D(uv, diffuseTexture);
	
	glm::vec3 fragmentlightDir = glm::normalize(sLight.Position() - pos);
	float theta = glm::dot(fragmentlightDir, lightDir);
	float attenuation = glm::clamp((theta - sLight.GetoutterCos()) / sLight.GetEpsilon(), 0.0f, 1.0f);

	glm::vec3 color = diffuseCoeff * diffuseColor * attenuation + ambient * diffuseColor;
	glm::vec3 lightColor = sLight.Color();
	gl_FragColor = { glm::min<float>(255.0, color.r * lightColor.r), glm::min<float>(255.0, color.g * lightColor.g), glm::min<float>(255.0, color.b * lightColor.b), 255};
	return false;
}