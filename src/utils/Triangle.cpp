#include "Triangle.h"

void SwapLineTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, TGAImage& image, TGAColor color)
{
	//1:����
	VertexSortVertical(v0, v1, v2);

	//2:ˮƽ���������,һ��Ϊ��,�Ȼ��°벿��
	SweepLine(v0, v1, v2, image, color);
}

//TODO:���������������㰴��yֵ��С��������
void VertexSortVertical(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2)
{
	if (v0.y > v1.y)
	{
		std::swap(v0, v1);
	}

	if (v0.y > v2.y)
	{
		std::swap(v0, v2);
	}

	if (v1.y > v2.y)
	{
		std::swap(v1, v2);
	}
}

//TODO:����ɨ���������(ɨ�����㷨)
void SweepLine(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2, TGAImage& image, TGAColor color)
{
	int downSegmentHeight = v1.y - v0.y + 1;
	int upSegmentHeight = v2.y - v1.y + 1;
	int totalHeight = v2.y - v0.y;
	for (uint32_t y = v0.y; y <= v1.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v0.y) / downSegmentHeight;
		glm::vec2 sideA = v0 + (v2 - v0) * alpha;
		glm::vec2 sideB = v0 + (v1 - v0) * beta;
		if (sideA.x > sideB.x)
		{
			std::swap(sideA, sideB);
		}
		for (uint32_t x = sideA.x; x <= sideB.x; x++)
		{
			image.set(x, y, color);
		}
	}

	//3:���ϰ벿��
	for (uint32_t y = v1.y; y <= v2.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v1.y) / upSegmentHeight;
		glm::vec2 sideA = v0 + (v2 - v0) * alpha;
		glm::vec2 sideB = v1 + (v2 - v1) * beta;
		if (sideA.x > sideB.x)
		{
			std::swap(sideA, sideB);
		}
		for (uint32_t x = sideA.x; x <= sideB.x; x++)
		{
			image.set(x, y, color);
		}
	}
}

glm::vec3 BaryCentric(glm::vec3* vertices, glm::vec2& p)
{	
	glm::vec2 v0 = vertices[0];
	glm::vec2 v1 = vertices[1];
	glm::vec2 v2 = vertices[2];
	glm::vec2 AB = v1 - v0;
	glm::vec2 AC = v2 - v0;
	glm::vec2 PA = v0 -  p;
	glm::vec3 A1 = glm::vec3{ AB.x, AC.x, PA.x };
	glm::vec3 A2 = glm::vec3{ AB.y, AC.y, PA.y };
	glm::vec3 uv1 = cross(A1, A2);
	if (uv1.z < 0)
	{
		return { -1, 1, 1 };
	}

	return { 1.0 - (uv1.x + uv1.y) / uv1.z, uv1.y / uv1.z, uv1.x / uv1.z };
}

BoundingBox GetBoundingBox(glm::vec3* vertices, glm::vec2 min, glm::vec2 max)
{
	BoundingBox bbox;
	bbox.min = max;
	bbox.max = min;
	for (uint32_t i = 0; i < 3; i++)
	{
		bbox.min.x = std::max(min.x, std::min(bbox.min.x, vertices[i].x));
		bbox.min.y = std::max(min.y, std::min(bbox.min.y, vertices[i].y));

		bbox.max.x = std::min(max.x, std::max(bbox.max.x, vertices[i].x));
		bbox.max.y = std::min(max.y, std::max(bbox.max.y, vertices[i].y));
	}
	return bbox;
}

void BaryCentricTriangle(glm::vec4* clipVertices, TGAImage& image, TGAImage& depthBuffer, const TGAColor& color, float* zBuffer, const glm::mat4& viewport, SDL_Renderer* sdlRenderer)
{
	glm::vec4 beforeDevision[3] = { viewport * clipVertices[0], viewport * clipVertices[1], viewport * clipVertices[2]};
	glm::vec3 afterDevision[3] = { beforeDevision[0] / beforeDevision[0].w, beforeDevision[1] / beforeDevision[1].w,beforeDevision[2] / beforeDevision[2].w };
	int width = image.get_width();
	int height = image.get_height();
	//1: ��ȡ��Χ��
	BoundingBox bbox = GetBoundingBox(afterDevision, { 0, 0 }, { width - 1.0, height - 1.0 });

	//2: ������Χ�л�ȡ��ǰ���ص����������
	for (uint32_t x = bbox.min.x; x<= bbox.max.x; x++)
	{
		for (uint32_t y = bbox.min.y; y<= bbox.max.y; y++) 
		{
			glm::vec2 p{(int)(x + 0.5f), (int)(y + 0.5f)};
			glm::vec3 bc_screen = BaryCentric(afterDevision, p);
			glm::vec3 bc_clip = { bc_screen.x / beforeDevision[0][3], bc_screen.y / beforeDevision[1][3], bc_screen.x / beforeDevision[2][3] };
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
			//3:�ж��Ƿ�����������,�������������,����������ɫ
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
			{
				continue;
			}
			//4:�ж�zbuffer
			float zBufferValue = glm::dot(glm::vec3(clipVertices[0].z, clipVertices[1].z, clipVertices[2].z), bc_clip);
			int bufferIndex = (int)(p.x + p.y * width);
			if (zBufferValue < zBuffer[bufferIndex])
			{
				zBuffer[bufferIndex] = zBufferValue;
				image.set(p.x, p.y, color);
				SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);
				//SDL����ԭ�������Ͻǣ���������½�ת�������Ͻ�
				glm::mat3 transform =
				{
					{ 1, 0, 0 },
					{ 0, -1, 0 },
					{ 0, height, 1 }
				};
				glm::vec3 pTransfomed = transform * glm::vec3(p.x, p.y, 1.0);
				SDL_RenderDrawPoint(sdlRenderer, pTransfomed.x, pTransfomed.y);
				//[-1, 1]
				float clipDepth = glm::dot(glm::vec3(afterDevision[0].z, afterDevision[1].z, afterDevision[2].z), bc_screen);
				float depth = (clipDepth + 1.0) * 0.5 * 255;
				depthBuffer.set(p.x, p.y, TGAColor(depth, depth, depth, 255));
			}
		}
	}
}