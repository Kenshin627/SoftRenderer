#include "Renderer.h"
#include "../window/window.h"
#include "model.h"
#include "../utils/Line.h"
#include "shader/FlatShader.h"
#include "shader/GouraudShader.h"
#include "shader/ToonShader.h"
#include "shader/PixelShader.h"
#include "shader/BlinnPhongShader.h"
#include "shader/TBNShader.h"
#include "shader/PointLightShader.h"
#include "shader/SpotlightShader.h"
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer(Window* device, uint32_t width, uint32_t height)
{
	presentDevice = device;
	frameBuffer.colorAttachment = TGAImage(width, height, TGAImage::RGB);
	frameBuffer.depthAttachment = TGAImage(width, height, TGAImage::GRAYSCALE);
	frameBuffer.zBuffer = new float[width * height];
	frameBuffer.width = width;
	frameBuffer.height = height;
	//models.emplace_back("source/models/head/head/african_head.obj");
	models.emplace_back("source/models/floor/floor.obj");
	//models.emplace_back("source/models/head/eye_inner/african_head_eye_inner.obj");
	//models.emplace_back("source/models/head/eye_outter/african_head_eye_outer.obj");
	//shader = std::make_unique<FlatShader>();
	//shader = std::make_unique<GouraudShader>();
	//shader = std::make_unique<ToonShader>();
	//shader = std::make_unique<PixelShader>();
	//shader = std::make_unique<BlinnPhongShader>();
	//shader = std::make_unique<TBNShader>();
	//shader = std::make_unique<PointLightShader>();
	shader = std::make_unique<SpotlightShader>();
	shader->baseColor = { 255,255,255 };
}

void Renderer::InitCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far)
{
	camera = Camera(eye, center, up, fov, aspectRatio, near, far);
	shader->viewprojectionMatrix = camera.GetViewProjection();
	shader->cameraPos = camera.GetPosition();
}

void Renderer::InitLight()
{
	dlight = DirectionLight({ 1.0, 1.0, 1.0 }, { 255, 255, 255 });
	pLight = PointLight({ 0, 1, 0 }, { 255, 255, 255 }, 1.0, 0.09, 0.032);
	sLight = SpotLight({ 0, 2.0, 0 }, { 0, 1, 0 }, { 255, 255, 255 }, glm::pi<float>() * 5.5f / 180.0f, glm::pi<float>() * 8.5f / 180.0f);
	shader->dLight = dlight;
	shader->pLight = pLight;
	shader->sLight = sLight;
}

void Renderer::Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	//MATRIX[COL][ROW]
	viewport =
	{
		{ width / 2, 0, 0, 0 },
		{ 0, height / 2, 0, 0 },
		{ 0, 0, 1, 0 },
		{ width / 2 + x, height / 2 + y, 0, 1 },
	};
}

void Renderer::rasterize(glm::vec4* clipVertices, glm::vec3* localCoords)
{
	glm::vec4 screenCoords[3] = { viewport * clipVertices[0], viewport * clipVertices[1], viewport * clipVertices[2] };
	glm::vec4 screenCoordsCliped[3] = { screenCoords[0] / screenCoords[0].w, screenCoords[1] / screenCoords[1].w, screenCoords[2] / screenCoords[2].w };

	glm::vec3 normal = glm::cross(localCoords[1] - localCoords[0], localCoords[2] - localCoords[0]);
	normal = glm::normalize(shader->invertTransposeModelMatrix * glm::normalize(normal));
	shader->flatShadeIntensity = glm::max<float>(0.0, glm::dot(normal, dlight.Direction()));
	BoundingBox bbox = GetBoundingBox(screenCoordsCliped);
	glm::vec4 gl_FragColor;
	for (uint32_t y = bbox.min.y; y <= bbox.max.y; y++)
	{
		for (uint32_t x = bbox.min.x; x <= bbox.max.x ; x++)
		{
			//定位像素位置，左下角->中心点
			glm::vec2 p { (int)(x + 0.5), (int)(y + 0.5) };
			glm::vec3 bc_screen = BaryCentric(screenCoordsCliped, p);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
			{
				continue;
			}			
			float depth = 1.0 / (bc_screen.x / screenCoords[0].z + bc_screen.y / screenCoords[1].z + bc_screen.z / screenCoords[2].z);
			shader->baryCentricCoords = { depth * bc_screen.x / screenCoords[0].z, depth * bc_screen.y / screenCoords[1].z, depth * bc_screen.z / screenCoords[2].z };
			uint32_t depthIndex = x + y * frameBuffer.width;
			if (depth < frameBuffer.zBuffer[depthIndex])
			{								
				if (shader->Fragment(gl_FragColor))
				{
					//Discard
					continue;
				}
				//绘制到屏幕
				presentDevice->DrawPoint(x, y, gl_FragColor);
				frameBuffer.zBuffer[depthIndex] = depth;
				//绘制到图片
				/*frameBuffer.colorAttachment.set(point.x, point.y, TGAColor(gl_FragColor.r, gl_FragColor.g, gl_FragColor.b, gl_FragColor.a));
				float depthVal = LinearDepth(camera.GetNear(), camera.GetFar(), depth) * 255;
				frameBuffer.depthAttachment.set(point.x, point.y, TGAColor(depthVal, depthVal, depthVal, 255));*/
			}
		}
	}
}

BoundingBox Renderer::GetBoundingBox(glm::vec4* vertices)
{
	BoundingBox bbox;
	glm::vec2 min {0, 0};
	glm::vec2 max { frameBuffer.width - 1, frameBuffer.height - 1 };

	bbox.min = max;
	bbox.max = min;
	for (auto& i : {0, 1, 2})
	{
		bbox.min.x = std::max(min.x, std::min(bbox.min.x, vertices[i].x));
		bbox.min.y = std::max(min.y, std::min(bbox.min.y, vertices[i].y));
		bbox.max.x = std::min(max.x, std::max(bbox.max.x, vertices[i].x));
		bbox.max.y = std::min(max.y, std::max(bbox.max.y, vertices[i].y));
	}
	return bbox;
}

glm::vec3 Renderer::BaryCentric(glm::vec4* vertices, glm::vec2& p)
{
	glm::vec2 v0 { vertices[0].x, vertices[0].y };
	glm::vec2 v1 { vertices[1].x, vertices[1].y };
	glm::vec2 v2 { vertices[2].x, vertices[2].y };
	glm::vec2 v01 = v1 - v0;
	glm::vec2 v02 = v2 - v0;
	glm::vec2 vp0 = v0 -  p;
	
	//TODO:　b(v1 - v0) + c(v2 - v0) + (v0 - p) = 0
	glm::vec3 mass = glm::cross(glm::vec3(v01.x, v02.x, vp0.x), glm::vec3(v01.y, v02.y, vp0.y));
	if (mass.z < 1e-2)
	{
		return { -1, 1, 1 };
	}
	return { 1.0 - (mass.x + mass.y) / mass.z, mass.x / mass.z, mass.y / mass.z };
}

void Renderer::Draw(uint64_t deltaTime)
{
	#pragma region INIT
	TGAColor red = TGAColor(255, 0, 0, 255);
	TGAColor green = TGAColor(0, 255, 0, 255);
	TGAColor blue = TGAColor(0, 0, 255, 255);
	TGAColor white = { 255, 255, 255, 255 };
	#pragma endregion

	#pragma region lINE
	//---------------------------------------------- Bresenham' line 
	//DrawLine2(20, 20, 80, 80, image, green);
	//DrawLine2(20, 20, 40, 80, image, red);
	//DrawLine3(20, 20, 30, 80, image, blue);
	//DrawLine3(20, 20, 80, 30, image, blue);
	/*DrawLine3(25, 25, 35, 85, image, green);
	DrawLine3(25, 25, 85, 35, image, green);

	DrawLine4(20, 20, 80, 30, image, red);
	DrawLine4(20, 20, 30, 80, image, red);
	image.flip_vertically();
	image.write_tga_file("output.tga");*/
	//---------------------------------------------- Bresenham' line
	#pragma endregion

	#pragma region LINE MESH RENDER
	/*for (int i = 0; i < model.nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			vec3 v0 = model.vert(i, j);
			vec3 v1 = model.vert(i, (j + 1) % 3);
			int x0 = (v0.x + 1.) * halfW;
			int y0 = (v0.y + 1.) * halfH;
			int x1 = (v1.x + 1.) * halfW;
			int y1 = (v1.y + 1.) * halfH;
			DrawLine4(x0, y0, x1, y1, image, white);
		}
	}
	image.flip_vertically();
	image.write_tga_file("LineMesh.tga");*/
	#pragma endregion

	/*glm::vec2 t0[3] = { glm::vec2{10, 70},   glm::vec2{50, 160},  glm::vec2{70, 80} };
glm::vec2 t1[3] = { glm::vec2{180, 50},  glm::vec2{150, 1},   glm::vec2{70, 180} };
glm::vec2 t2[3] = { glm::vec2{180, 150}, glm::vec2{120, 160}, glm::vec2{130, 180} };*/

	#pragma region SWAPLINE TRIANGLE
	/*SwapLineTriangle(t0[0], t0[1], t0[2], image, red);
	SwapLineTriangle(t1[0], t1[1], t1[2], image, blue);
	SwapLineTriangle(t2[0], t2[1], t2[2], image, green);
	image.flip_vertically();
	image.write_tga_file("output.tga");*/
	#pragma endregion
	
	#pragma region BARYCENTRIC TRIANGLE
	/*BaryCentricTriangle(t0, image, red);
	BaryCentricTriangle(t1, image, blue);
	BaryCentricTriangle(t2, image, green);
	image.flip_vertically();
	image.write_tga_file("output.tga");*/
	#pragma endregion
	
	#pragma region FACE SHADING
	glm::vec3 localCoords[3];
	glm::vec4 clipCoords[3];
	glm::vec2 uvs[3];

	//test-移动光源位置
	shader->sLight.SetPosition(shader->sLight.Position() + glm::vec3(0.001, 0, 0));
	//test-移动光源方向
	//shader->sLight.SetDirection(glm::normalize(shader->sLight.Direction() + glm::vec3(glm::cos(0.01f) * 0.01, glm::sin(0.01f) * 0.01, 0)));
	for (uint32_t i = 0; i < models.size(); i++)
	{
		Model model = models[i];
	/*	shader->modelMatrix = glm::rotate(shader->modelMatrix, 3.1415926f / 4.0f * 0.1f, glm::vec3(0, 1, 0));
		shader->invertTransposeModelMatrix = glm::transpose(glm::inverse(shader->modelMatrix));*/
		
		shader->SetUniformSampler(0, model.diffuse());
		shader->SetUniformSampler(1, model.specular());
		shader->SetUniformSampler(2, model.normalMap());
		for (int i = 0; i < model.nfaces(); i++) {
			for (int j = 0; j < 3; j++)
			{
				localCoords[j] = model.vert(i, j);
				glm::vec3 normal = model.normal(i, j);
				uvs[j] = model.uv(i, j);
				normal = glm::normalize(normal);				
				VertexAttribute vertex { glm::vec4(localCoords[j].x, localCoords[j].y, localCoords[j].z, 1.0), normal, uvs[j] };
				shader->Vertex(clipCoords[j], vertex, j);
			}
			computeTBN(localCoords, uvs, shader);
			rasterize(clipCoords, localCoords);
		}
	}	
	frameBuffer.colorAttachment.write_tga_file("color.tga");
	frameBuffer.depthAttachment.write_tga_file("depth.tga");
	#pragma endregion
}

void Renderer::computeTBN(glm::vec3* positions, glm::vec2* uvs, std::unique_ptr<Shader>& shader)
{
	glm::vec3 e1 = positions[1] - positions[0];
	glm::vec3 e2 = positions[2] - positions[0];
	glm::vec2 deltaUV1 = uvs[1] - uvs[0];
	glm::vec2 deltaUV2 = uvs[2] - uvs[0];
	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * e1.x - deltaUV1.y * e2.x);
	tangent.y = f * (deltaUV2.y * e1.y - deltaUV1.y * e2.y);
	tangent.z = f * (deltaUV2.y * e1.z - deltaUV1.y * e2.z);
	tangent = glm::normalize(tangent);

	glm::vec3 bitangent;
	bitangent.x = f * (-deltaUV2.x * e1.x + deltaUV1.x * e2.x);
	bitangent.y = f * (-deltaUV2.x * e1.y + deltaUV1.x * e2.y);
	bitangent.z = f * (-deltaUV2.x * e1.z + deltaUV1.x * e2.z);
	bitangent = glm::normalize(bitangent);
	shader->tangent = tangent;
	shader->bitangent = bitangent;
}

float Renderer::LinearDepth(float near, float far, float depth)
{
	depth = (depth + 1.0) / 2.0;
	return (2.0 * near) / (far + near - depth * (far - near));
}

void Renderer::Clear()
{
	frameBuffer.Clear();
}
