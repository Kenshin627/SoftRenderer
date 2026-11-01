#include "ToonEdgeShader.h"
#include <limits>

void ToonEdgeShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	//post-process pass: nothing to transform
}

bool ToonEdgeShader::Fragment(glm::vec4& gl_FragColor)
{
	//tutorial toon chapter, Sobel kernels:
	// Kx = [-1 0 +1]   Ky = [-1 -2 -1]
	//      [-2 0 +2]        [ 0  0  0]
	//      [-1 0 +1]        [+1 +2 +1]
	//edge strength = sqrt(Gx^2 + Gy^2); the background (empty depth = FLT_MAX) gives an
	//astronomical contrast against the scene, so silhouettes respond strongest
	int x = (int)currentPixel.x;
	int y = (int)currentPixel.y;
	int w = prePassColorAttachment.get_width();
	int h = prePassColorAttachment.get_height();
	auto depthAt = [&](int px, int py) -> float
	{
		px = glm::clamp(px, 0, w - 1);
		py = glm::clamp(py, 0, h - 1);
		return prePassDepth[px + py * w];
	};
	float gx = -depthAt(x - 1, y - 1) + depthAt(x + 1, y - 1)
	           -2.0f * depthAt(x - 1, y) + 2.0f * depthAt(x + 1, y)
	           -depthAt(x - 1, y + 1) + depthAt(x + 1, y + 1);
	float gy = -depthAt(x - 1, y - 1) - 2.0f * depthAt(x, y - 1) - depthAt(x + 1, y - 1)
	           +depthAt(x - 1, y + 1) + 2.0f * depthAt(x, y + 1) + depthAt(x + 1, y + 1);
	float edge = glm::sqrt(gx * gx + gy * gy);
	if (edge > edgeThreshold)
	{
		gl_FragColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // ink line
		return false;
	}
	TGAColor c = prePassColorAttachment.get(x, y);
	gl_FragColor = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f };
	return false;
}
