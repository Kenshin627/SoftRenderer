#include "Window.h"
#include <iostream>

Window::Window(const char* title, uint32_t w, uint32_t h):width(w),height(h)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout << "SDL init failed!" << std::endl;
	}
	windowHandle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	drawHandle = SDL_CreateRenderer(windowHandle, -1, 0);
	glm::mat3 yInverse =
	{
		{ 1, 0,  0 },
		{ 0, -1, 0 },
		{ 0, 0,  1 }
	};
	glm::mat3 translateH =
	{
		{ 1, 0,      0 },
		{ 0, 1,      0 },
		{ 0, height, 1 }
	};
	sdlTransform = translateH * yInverse;
}

Window::~Window()
{
	ShutDowm();
}

void Window::ShutDowm()
{
	SDL_DestroyRenderer(drawHandle);
	SDL_DestroyWindow(windowHandle);
	SDL_Quit();
}

void Window::Run()
{
	while (!shouldClosed)
	{
		SDL_Event event;
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT)
		{
			shouldClosed = true;
			return;
		}
		Clear();
		softRenderer->Clear();
		softRenderer->Draw();
		SDL_RenderPresent(drawHandle);
	}
}

void Window::DrawPoint(int x, int y, const glm::vec3& color, bool flip)
{
	glm::vec2 sdlPoint;
	if (flip)
	{
		sdlPoint = sdlTransform * glm::vec3(x, y, 1.0f);
	}
	else {
		sdlPoint = glm::vec2(x, y);
	}
	SDL_SetRenderDrawColor(drawHandle, (Uint8)color.r, (Uint8)color.g, (Uint8)color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawPoint(drawHandle, (int)sdlPoint.x, (int)sdlPoint.y);
}

void Window::Clear()
{
	SDL_SetRenderDrawColor(drawHandle, (Uint8)clearColor.r, (Uint8)clearColor.g, (Uint8)clearColor.b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(drawHandle);
}