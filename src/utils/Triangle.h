#pragma once
#include <tgaimage/tgaimage.h>
#include "../renderer/math.h"
void Triangle(vec2 v0, vec2 v1, vec2 v2, TGAImage& image, TGAColor color);

void VertexSortVertical(vec2& v0, vec2& v1, vec2& v2);

void SweepLine(vec2& v0, vec2& v1, vec2& v2, TGAImage& image, TGAColor color);