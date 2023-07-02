#include <iostream>
#include "tgaimage/tgaimage.h"
#include "renderer/model.h"
#include "sdl2/include/SDL.h"

#include "utils/Line.h"
#include "utils/Triangle.h"

void draw(TGAImage& image);

int main(int argc, char* argv[])
{
	SDL_Window* window;
	int screenWidth = 1000;
	int screenHeight = 1000;

	SDL_Renderer* renderer;
	SDL_Texture* texture;

	bool shouldClose = false;
	int suceess = SDL_Init(SDL_INIT_VIDEO);
	if (suceess != 0)
	{
		std::cout << "sdl init failed!" << std::endl;
	}

	window = SDL_CreateWindow("SoftRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
	
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
	/*while (!shouldClose)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) 
			{
				shouldClose = true;
			}
		}*/
		SDL_RenderClear(renderer);
		TGAImage image(screenWidth, screenHeight, TGAImage::RGBA);
		draw(image);
		/*SDL_UpdateTexture(texture, nullptr, image.GetRaw(), screenWidth * 3);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);*/
	/*}
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();*/
	
	return 0;
}

void draw(TGAImage& image)
{
	#pragma region INIT
	TGAColor red = TGAColor(255, 0, 0, 255);
	TGAColor green = TGAColor(0, 255, 0, 255);
	TGAColor blue = TGAColor(0, 0, 255, 255);
	TGAColor white = { 255, 255, 255, 255 };
	uint32_t width = image.get_width();
	uint32_t height = image.get_height();
	uint32_t halfW = width / 2;
	uint32_t halfH = height / 2;
	Model model{ "source/models/diablo/d3.obj" };
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
	glm::vec2 screenCoords[3];
	glm::vec3 lightDir{ 0.0, 0.0, -1.0 };
	
	for (int i = 0; i < model.nfaces(); i++) {
		for (int j = 0; j < 3; j++)
		{
			auto vertex = model.vert(i, j);
			glm::vec3 pos = { vertex .x, vertex.y, vertex.z };
			worldCoords[j] = pos;
			screenCoords[j] = { (pos.x + 1) * halfW,(pos.y + 1) * halfH };
		}
		glm::vec3 normal = glm::cross(worldCoords[2] - worldCoords[0], worldCoords[1] - worldCoords[0]);
		normal = glm::normalize(normal);
		float intensity = glm::dot(normal, lightDir);
		if (intensity > 0)
		{
			BaryCentricTriangle(screenCoords, image, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255));
		}
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	#pragma endregion

}