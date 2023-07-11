#include "GouraudShader.h"

void GouraudShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	//�𶥵������գ������ռ�������fragment���������ֵ
	vertexIntensity[gl_VertexIndex] = glm::max<float>(0.0, glm::dot(vertex.normal, dLight.Direction()));
	gl_Position = viewprojectionMatrix * vertex.position;
}

//return discard
bool GouraudShader::Fragment(glm::vec4& gl_FragColor)
{
	//���������ֵ���㵱ǰ���ص����ǿ��
	float pixelIntensity = glm::max<float>(0.0, glm::dot(vertexIntensity, baryCentricCoords));
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}