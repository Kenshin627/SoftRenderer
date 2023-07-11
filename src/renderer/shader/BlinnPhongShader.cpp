#include "BlinnPhongShader.h"
#include <glm/gtc/matrix_transform.hpp>

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	positions[gl_VertexIndex] = modelMatrix * vertex.position;
	normals[gl_VertexIndex] = glm::normalize(invertTransposeModelMatrix * vertex.normal);
	uvs[gl_VertexIndex] = vertex.uv;
	gl_Position = viewprojectionMatrix * modelMatrix * vertex.position;
}

//return discard
bool BlinnPhongShader::Fragment(glm::vec4& gl_FragColor)
{
	//重心坐标插值计算当前像素点法线，逐像素计算光照
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	glm::vec2 uv = uvs[0] * baryCentricCoords.x + uvs[1] * baryCentricCoords.y + uvs[2] * baryCentricCoords.z;
	glm::vec3 pos = positions[0] * baryCentricCoords.x + positions[1] * baryCentricCoords.y + positions[2] * baryCentricCoords[2];
	n = glm::normalize(n);

	glm::vec3 h = glm::normalize((dLight.Direction() + glm::normalize(cameraPos - pos)));
	float diffuseCoeff = glm::max<float>(0.0, glm::dot(n, dLight.Direction()));
	glm::vec3 diffuseColor = Sampler2D(uv, diffuseTexture);
	float samplerSpecular = Sampler2D(uv, specularTexture).r;
	float specularCoeff = glm::pow(glm::max<float>(0.0, glm::dot(n, h)), 256) * samplerSpecular;
	float blinnPhongFactor = specularCoeff + diffuseCoeff;
	glm::vec3 color = blinnPhongFactor * diffuseColor;
	glm::vec3 lightColor = dLight.Color();
	gl_FragColor = { glm::min<float>(255.0, color.r * lightColor.r), glm::min<float>(255.0, color.g * lightColor.g), glm::min<float>(255.0, color.b * lightColor.b), 255};
	return false;
}