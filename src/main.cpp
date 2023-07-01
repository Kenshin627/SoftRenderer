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
	int screenWidth = 1024;
	int screenHeight = 768;

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
	while (!shouldClose)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) 
			{
				shouldClose = true;
			}
		}
		SDL_RenderClear(renderer);
		TGAImage image(screenWidth, screenHeight, 4);
		draw(image);
		SDL_UpdateTexture(texture, nullptr, image.GetRaw(), screenWidth * 4);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}

void draw(TGAImage& image)
{
	#pragma region lINE
	//---------------------------------------------- Bresenham' line 
	//TGAImage image(100, 100, TGAImage::RGB);
	//TGAColor red = TGAColor(255, 0, 0, 255);
	//TGAColor green = TGAColor(0, 255, 0, 255);
	//TGAColor blue = TGAColor(0, 0, 255, 255);
	////DrawLine2(20, 20, 80, 80, image, green);
	////DrawLine2(20, 20, 40, 80, image, red);
	////DrawLine3(20, 20, 30, 80, image, blue);
	////DrawLine3(20, 20, 80, 30, image, blue);
	//DrawLine3(25, 25, 35, 85, image, green);
	//DrawLine3(25, 25, 85, 35, image, green);

	//DrawLine4(20, 20, 80, 30, image, red);
	//DrawLine4(20, 20, 30, 80, image, red);
	//image.flip_vertically();
	//image.write_tga_file("output.tga");
	//---------------------------------------------- Bresenham' line

	//---------------------------------------------- line Mesh renderer
	/*uint32_t width = 1000;
	uint32_t height = 1000;
	uint32_t halfW = image.width() / 2;
	uint32_t halfH = image.height() / 2;
	TGAColor white = { 255, 255, 255, 255, 4 * 8 };
	Model model{ "source/models/diablo/diablo3_pose.obj" };
	for (int i = 0; i < model.nfaces(); i++) {
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
	image.flip_vertically();*/
	//---------------------------------------------- line Mesh renderer
	#pragma endregion

	#pragma region TRIANGLE

	vec2 t0[3] = { vec2{10, 70},   vec2{50, 160},  vec2{70, 80} };
	vec2 t1[3] = { vec2{180, 50},  vec2{150, 1},   vec2{70, 180} };
	vec2 t2[3] = { vec2{180, 150}, vec2{120, 160}, vec2{130, 180} };

	TGAColor red = { 255, 0, 0, 255, 4 * 8 };
	TGAColor blue = { 0, 0, 255, 255, 4 * 8 };
	TGAColor green = { 0, 255, 0, 255, 4 * 8 };
	Triangle(t0[0], t0[1], t0[2], image, red);
	Triangle(t1[0], t1[1], t1[2], image, blue);
	Triangle(t2[0], t2[1], t2[2], image, green);
	#pragma endregion

}