#include "BlinnPhongShader.h"

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	normals[gl_VertexIndex] = vertex.normal;
	uvs[gl_VertexIndex] = vertex.uv;
	gl_Position = modelViewprojection * vertex.position;
}

//return discard
bool BlinnPhongShader::Fragment(glm::vec4& gl_FragColor, float intensity)
{
	//重心坐标插值计算当前像素点法线，逐像素计算光照
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	glm::vec2 uv = uvs[0] * baryCentricCoords.x + uvs[1] * baryCentricCoords.y + uvs[2] * baryCentricCoords.z;
	n = glm::normalize(n);
	float pixelIntensity = glm::max<float>(0.0, glm::dot(n, lightDir));
	TGAColor diffuse = diffuseTexture.get(uv.x * diffuseTexture.get_width(), uv.y * diffuseTexture.get_height());
	gl_FragColor = { diffuse.r * pixelIntensity, diffuse.g * pixelIntensity, diffuse.b * pixelIntensity, 255 };
	return false;
}