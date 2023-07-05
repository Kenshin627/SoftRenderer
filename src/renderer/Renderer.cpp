#include "Renderer.h"
#include "model.h"
#include "../utils/Line.h"


Renderer::Renderer(uint32_t width, uint32_t height)
{
	frameBuffer = TGAImage(width, height, TGAImage::RGB);
	depthBuffer = TGAImage(width, height, TGAImage::GRAYSCALE);
	zBuffer = new float[width * height];
	screenwidth = width;
	screenheight = height;
	pixelCount = width * height;
	halfWidth = width / 2;
	halfHeight = height / 2;
	models.emplace_back("source/models/head/head.obj");
}

void Renderer::InitCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far)
{
	camera = Camera(eye, center, up, fov, aspectRatio, near, far);
}

void Renderer::Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	viewport =
	{
		{ width / 2, 0, 0, 0 },
		{ 0, height / 2, 0, 0 },
		{ 0, 0, 1, 0 },
		{ width / 2 + x, height / 2 + y, 0, 1 },
	};
}

void Renderer::Draw(SDL_Renderer* renderer)
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
	glm::vec3 worldCoords[3];
	glm::vec4 clipCoords[3];
	glm::vec3 lightDir{ 0.0, 0.0, -1.0 };

	for (uint32_t i = 0; i < models.size(); i++)
	{
		Model model = models[i];
		for (int i = 0; i < model.nfaces(); i++) {
			for (int j = 0; j < 3; j++)
			{
				auto vertex = model.vert(i, j);
				glm::vec3 pos = { vertex.x, vertex.y, vertex.z };
				worldCoords[j] = pos;
				clipCoords[j] = camera.GetProjection() * camera.GetView() * glm::vec4(pos.x, pos.y, pos.z, 1.0);
			}
			glm::vec3 normal = glm::cross(worldCoords[2] - worldCoords[0], worldCoords[1] - worldCoords[0]);
			normal = glm::normalize(normal);
			float intensity = glm::dot(normal, lightDir);
			if (intensity > 0)
			{
				BaryCentricTriangle(clipCoords, frameBuffer, depthBuffer, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255), zBuffer, viewport, renderer);
			}
		}
	}
	frameBuffer.flip_vertically();
	depthBuffer.flip_vertically();
	frameBuffer.write_tga_file("color.tga");
	depthBuffer.write_tga_file("depth.tga");
	#pragma endregion
}

void Renderer::Clear()
{
	frameBuffer.clear();
	depthBuffer.clear();
	for (uint32_t i = 0; i < screenwidth * screenheight; i++)
	{
		zBuffer[i] = std::numeric_limits<double>::max();
	}
}

Renderer::~Renderer()
{
	delete[] zBuffer;
}