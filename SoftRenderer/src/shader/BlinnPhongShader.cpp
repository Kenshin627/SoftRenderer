#include "BlinnPhongShader.h"

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, itModel * vertex.tangent, itModel * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool BlinnPhongShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec2 uv = baryCentric.x * triangle.vertices[0].uv + baryCentric.y * triangle.vertices[1].uv + baryCentric.z * triangle.vertices[2].uv;
	glm::vec3 worldPos = baryCentric.x * triangle.vertices[0].position + baryCentric.y * triangle.vertices[1].position + baryCentric.z * triangle.vertices[2].position;	
	glm::vec3 worldNormal = glm::normalize(baryCentric.x * triangle.vertices[0].normal + baryCentric.y * triangle.vertices[1].normal + baryCentric.z * triangle.vertices[2].normal);

	//构造TBN:插值切线/副切线 + 插值法线 = 原始 Darboux 帧(与参考实现 D={t,b,n} 一致)
	glm::vec3 N = worldNormal;
	glm::vec3 T = glm::normalize(baryCentric.x * triangle.vertices[0].tangent + baryCentric.y * triangle.vertices[1].tangent + baryCentric.z * triangle.vertices[2].tangent);
	glm::vec3 B = glm::normalize(baryCentric.x * triangle.vertices[0].bitangent + baryCentric.y * triangle.vertices[1].bitangent + baryCentric.z * triangle.vertices[2].bitangent);
	glm::mat3 TBN = {
		T, B, N
	};
	glm::vec3 samplerN = glm::normalize(Sampler2D(uv, tbnNormalTexture)) * 2.0f - 1.0f;
	glm::vec3 normal = glm::normalize(TBN * samplerN);

	//光照方向取反，从像素出发
	glm::vec3 lightDir = glm::normalize(-light->Direction());
	glm::vec3 lightColor = light->Color();
	
	//diffuse
	glm::vec3 diffuseColor = Sampler2D(uv, diffuseTexture); //[0-1]
	float diffuse = glm::max(0.0f, glm::dot(lightDir, normal));
	diffuseColor = diffuse * diffuseColor;

	//specular
	glm::vec3 specularColor;
	glm::vec3 viewDir = glm::normalize(camPos - worldPos);
	glm::vec3 h = glm::normalize((viewDir + lightDir));
	float specular = glm::pow(glm::max(0.0f, glm::dot(h, normal)), 32);
	if (specularTexture != nullptr)
	{		
		float samplerSpecular = Sampler2D(uv, specularTexture).b;//[0-1]
		specularColor = glm::vec3(samplerSpecular);
		specularColor = specularColor * specular;
	}
	else {
		specularColor = glm::vec3(specular);
	}
	

	//ambient
	glm::vec3 ambientColor = glm::vec3(0.08f);

	glm::vec3 color = (diffuseColor + specularColor + ambientColor) * lightColor;
	color.r = glm::min(1.0f, color.r);
	color.g = glm::min(1.0f, color.g);
	color.b = glm::min(1.0f, color.b);
	gl_FragColor = { color, 1.0f };
	return false;
}