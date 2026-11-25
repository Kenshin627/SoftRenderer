#pragma once
#include "../renderer/Renderer.h"
#include <SDL.h>

class Window
{
public:
	Window(const char* title, uint32_t width, uint32_t height);
	~Window();
	bool Show(const TGAImage& frame);
private:
	SDL_Window* windowHandle = nullptr;
	SDL_Renderer* drawHandle = nullptr;
};
