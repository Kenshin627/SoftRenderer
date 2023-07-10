#include "BlinnPhongShader.h"
#include <glm/gtc/matrix_transform.hpp>

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	positions[gl_VertexIndex] = vertex.position;
	normals[gl_VertexIndex] = vertex.normal;
	uvs[gl_VertexIndex] = vertex.uv;
	gl_Position = modelViewprojection * vertex.position;
	//存储ndc空间顶点坐标，用于fragment计算TBN矩阵
}

//return discard
bool BlinnPhongShader::Fragment(glm::vec4& gl_FragColor, float intensity)
{
	//重心坐标插值计算当前像素点法线，逐像素计算光照
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	glm::vec2 uv = uvs[0] * baryCentricCoords.x + uvs[1] * baryCentricCoords.y + uvs[2] * baryCentricCoords.z;
	glm::vec3 pos = positions[0] * baryCentricCoords.x + positions[1] * baryCentricCoords.y + positions[2] * baryCentricCoords[2];
	n = glm::normalize(n);


	glm::vec3 h = glm::normalize((lightDir + glm::normalize(cameraPos - pos)));

	float diffuse = glm::max<float>(0.0, glm::dot(n, lightDir));
	TGAColor diffuseColor = diffuseTexture.get(uv.x * diffuseTexture.get_width(), uv.y * diffuseTexture.get_height());
	float specularFactor = specularTexture.get(uv.x * specularTexture.get_width(), uv.y * specularTexture.get_height()).b / 255.0;
	float specular = glm::pow(glm::max<float>(0.0, glm::dot(n, h)), 256) * specularFactor;
	float blinnPhongFactor = glm::min<float>(1.0, specular + diffuse);
	gl_FragColor = { blinnPhongFactor * diffuseColor.r, blinnPhongFactor * diffuseColor.g, blinnPhongFactor * diffuseColor.b, 255};
	return false;
}