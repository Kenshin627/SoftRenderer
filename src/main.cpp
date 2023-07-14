#include <iostream>
#include "tgaimage/tgaimage.h"
#include "window/window.h"
#include <glm/gtc/type_ptr.hpp>

static constexpr uint32_t SCREEN_HEIGHT = 1000;
static constexpr uint32_t ASPECT = 16.0 / 9.0;
static constexpr uint32_t SCREEN_WIDTH = SCREEN_HEIGHT * ASPECT;

int main(int argc, char* argv[])
{	
	Window window{ "SoftRenderer", SCREEN_WIDTH, SCREEN_HEIGHT };
	Renderer renderer(&window, SCREEN_WIDTH, SCREEN_HEIGHT);
	renderer.InitLight();
	renderer.InitCamera({ 1, 1, 3 }, { 0,0,0 }, { 0,1,0 }, 1.0f / glm::four_over_pi<float>(), ASPECT, 0.01, 10.0);
	renderer.Viewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	window.SetRenderContext(&renderer);
	window.Run();
	return 0;
}
