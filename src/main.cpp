#include <iostream>
#include "tgaimage/tgaimage.h"
#include "sdl2/include/SDL.h"
#include "renderer/Renderer.h"

int main(int argc, char* argv[])
{
	constexpr uint32_t SCREEN_WIDTH = 1000;
	constexpr uint32_t SCREEN_HEIGHT = 1000;
	SDL_Window* window;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* texture;
	Renderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
	renderer.InitCamera({ 2, 2, 4 }, { 0,0,0 }, { 0,1,0 }, 3.14159265358 / 4.0, 1, 0.01, 100.0);
	renderer.Viewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	bool shouldClose = false;
	int suceess = SDL_Init(SDL_INIT_VIDEO);
	if (suceess != 0)
	{
		std::cout << "sdl init failed!" << std::endl;
	}

	window = SDL_CreateWindow("SoftRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	sdlRenderer = SDL_CreateRenderer(window, -1, 0);
	texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 1);

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
		SDL_RenderClear(sdlRenderer);
		renderer.Clear();
		renderer.Draw();
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
