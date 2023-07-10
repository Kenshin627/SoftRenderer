#include "BlinnPhongShader.h"
#include <glm/gtc/matrix_transform.hpp>

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	postions[gl_VertexIndex] = vertex.position;
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
	glm::vec3 pos = postions[0] * baryCentricCoords.x + postions[1] * baryCentricCoords.y + postions[2] * baryCentricCoords[2];
	n = glm::normalize(n);

	/*
	 *TBN矩阵计算
	 */
	glm::mat3 A = {
		postions[1] - postions[0],
		postions[2] - postions[0],
		n
	};
	glm::mat3 Ainvert = glm::inverse(A);
	glm::vec3 T = glm::normalize(Ainvert * glm::vec3(uvs[1].x - uvs[0].x, uvs[2].x - uvs[0].x, 0));
	glm::vec3 B = glm::normalize(Ainvert * glm::vec3(uvs[1].y - uvs[0].y, uvs[2].y - uvs[0].y, 0));
	glm::vec3 N = n;

	glm::mat3 TBN = { T, B, N };
	TGAColor normalVal = normalMap.get(uv.x * normalMap.get_width(), uv.y * normalMap.get_height());
	glm::vec3 normal { (normalVal.r / 255.0) * 2.0 - 1.0, (normalVal.g / 255.0) * 2.0 - 1.0, (normalVal.b / 255.0) * 2.0 - 1.0 };
	glm::vec3 n2 = TBN * glm::normalize(normal);
	n2 = glm::normalize(n2);

	glm::vec3 h = glm::normalize((lightDir + glm::normalize(cameraPos - pos)));

	float diffuse = glm::max<float>(0.0, glm::dot(n2, lightDir));
	TGAColor diffuseColor = diffuseTexture.get(uv.x * diffuseTexture.get_width(), uv.y * diffuseTexture.get_height());
	float specularFactor = specularTexture.get(uv.x * specularTexture.get_width(), uv.y * specularTexture.get_height()).b / 255.0;
	float specular = glm::pow(glm::max<float>(0.0, glm::dot(n2, h)), 256) * specularFactor;

	gl_FragColor = { (specular + diffuse) * diffuseColor.r, (specular + diffuse)* diffuseColor.g, (specular + diffuse) * diffuseColor.b, 255};
	return false;
}