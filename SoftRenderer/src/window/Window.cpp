#include "Window.h"

Window::Window(const char* title, uint32_t width, uint32_t height)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) return;
	windowHandle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (windowHandle) drawHandle = SDL_CreateRenderer(windowHandle, -1, 0);
}

Window::~Window()
{
	SDL_DestroyRenderer(drawHandle);
	SDL_DestroyWindow(windowHandle);
	SDL_Quit();
}

bool Window::Show(const TGAImage& frame)
{
	if (!drawHandle) return false;
	SDL_Texture* texture = SDL_CreateTexture(drawHandle, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STATIC,
		frame.get_width(), frame.get_height());
	if (!texture) return false;
	bool ok = SDL_UpdateTexture(texture, nullptr, frame.buffer(), frame.get_width() * 3) == 0;
	SDL_Event event;
	while (ok)
	{
		ok = SDL_RenderCopy(drawHandle, texture, nullptr, nullptr) == 0;
		SDL_RenderPresent(drawHandle);
		if (!SDL_WaitEvent(&event)) { ok = false; break; }
		if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) break;
	}
	SDL_DestroyTexture(texture);
	return ok;
}
