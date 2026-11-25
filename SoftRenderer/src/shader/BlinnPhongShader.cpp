#include "BlinnPhongShader.h"

void BlinnPhongShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	triangle.vertices[vertexIndex] = { model * glm::vec4(vertex.position, 1.0), glm::normalize(itModel * vertex.normal), vertex.uv, glm::mat3(model) * vertex.tangent, glm::mat3(model) * vertex.bitangent };
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}

bool BlinnPhongShader::Fragment(glm::vec4& gl_FragColor)
{
	glm::vec2 uv = baryCentric.x * triangle.vertices[0].uv + baryCentric.y * triangle.vertices[1].uv + baryCentric.z * triangle.vertices[2].uv;
	glm::vec3 worldPos = baryCentric.x * triangle.vertices[0].position + baryCentric.y * triangle.vertices[1].position + baryCentric.z * triangle.vertices[2].position;	


	glm::vec3 normal = SurfaceNormal(uv);

	//光照方向取反，从像素出发
	glm::vec3 lightDir = glm::normalize(-light->Direction());
	glm::vec3 lightColor = light->Color();
	
	//diffuse
	glm::vec3 diffuseColor = SampleColor(uv, diffuseTexture); //[0-1]
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
	if (hdr) color += SampleEmission(uv);
	gl_FragColor = { color, 1.0f };
	return false;
}