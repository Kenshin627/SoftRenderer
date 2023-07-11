#pragma once
#include <sdl2/include/SDL.h>
#include "../renderer/Renderer.h"

class Window
{
public:
	Window(const char* title, uint32_t width, uint32_t height);
	~Window();
	void SetRenderContext(Renderer* renderer);
	void Run();
	void DrawPoint(int x, int y, const glm::vec4& color);
	void Clear();
	void Destroy();

	SDL_Renderer* GetSDLRenderer() { return sdlRenderer; }
private:
	SDL_Window* windowHandle = nullptr;
	SDL_Renderer* sdlRenderer = nullptr;
	Renderer* softRenderer = nullptr;
	bool closed = false;
	glm::mat3 screenTransform;
};