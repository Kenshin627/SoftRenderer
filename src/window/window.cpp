#include "window.h"
#include <iostream>

Window::Window(const char* title, uint32_t width, uint32_t height)
{
	if(SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout << "sdl init failed!" << std::endl;
	}

	windowHandle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	sdlRenderer = SDL_CreateRenderer(windowHandle, -1, 0);
	screenTransform = glm::mat3({ 1, 0, 0 }, { 0, 1, 0 }, { 0, height, 1 }) * glm::mat3({ 1, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 });
}

void Window::SetRenderContext(Renderer* renderer)
{
	softRenderer = renderer;
}

void Window::Run()
{
	SDL_Event event;
	while (!closed)
	{
		uint64_t currentTime = SDL_GetPerformanceCounter();
		deltaTime = (currentTime - preFrameTime) / (float)SDL_GetPerformanceFrequency();
		preFrameTime = currentTime;
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			closed = true;			
		}
		else
		{
			Clear();
			softRenderer->Clear();
			softRenderer->Draw(deltaTime);
			SDL_RenderPresent(sdlRenderer);
		}
	}
}

void Window::DrawPoint(int x, int y, const glm::vec4& color)
{
	glm::vec3 p = screenTransform * glm::vec3(x, y, 1);
	SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(sdlRenderer, p.x, p.y);
}

void Window::Clear()
{
	SDL_SetRenderDrawColor(sdlRenderer, 20, 20, 20, 255);
	SDL_RenderClear(sdlRenderer);
}

void Window::Destroy()
{
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(windowHandle);
	SDL_Quit();
}

Window::~Window()
{
	Destroy();
}
