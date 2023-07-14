#include "GouraudShader.h"

void GouraudShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	//�𶥵������գ������ռ�������fragment���������ֵ
	glm::vec3 lightDir = glm::normalize(-dLight.Direction());
	vertexIntensity[gl_VertexIndex] = glm::max<float>(0.0, glm::dot(glm::normalize(invertTransposeModelMatrix * vertex.normal), lightDir));
	gl_Position = viewprojectionMatrix * modelMatrix * vertex.position;
}

//return discard
bool GouraudShader::Fragment(glm::vec4& gl_FragColor)
{
	//���������ֵ���㵱ǰ���ص����ǿ��
	float pixelIntensity = glm::max<float>(0.0, glm::dot(vertexIntensity, baryCentricCoords));
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}