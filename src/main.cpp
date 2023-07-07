#include <iostream>
#include "tgaimage/tgaimage.h"
#include "renderer/Renderer.h"

int main(int argc, char* argv[])
{	
	constexpr uint32_t SCREEN_HEIGHT = 1000;
	constexpr uint32_t ASPECT = 16.0 / 9.0;
	constexpr uint32_t SCREEN_WIDTH = SCREEN_HEIGHT * ASPECT;
	SDL_Window* window;
	SDL_Renderer* sdlRenderer;
		
	bool shouldClose = false;
	int suceess = SDL_Init(SDL_INIT_VIDEO);
	if (suceess != 0)
	{
		std::cout << "sdl init failed!" << std::endl;
	}

	window = SDL_CreateWindow("SoftRenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	sdlRenderer = SDL_CreateRenderer(window, -1, 0);
	Renderer renderer(sdlRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	renderer.InitCamera({ 1, 1, 3 }, { 0,0,0 }, { 0,1,0 }, 3.14159265358 / 4.0, ASPECT, 0.01, 10.0);
	renderer.Viewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
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
		SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 1);
		SDL_RenderClear(sdlRenderer);
		renderer.Clear();
		renderer.Draw();
		SDL_RenderPresent(sdlRenderer);
	}
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
