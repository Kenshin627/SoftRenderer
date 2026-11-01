#include "Renderer.h"
#include "Triangle.h"
#include "../window/Window.h"
#include "../shader/GouraudShader.h"
#include "../shader/PixelShader.h"
#include "../shader/WireFrameShader.h"
#include "../shader/BlinnPhongShader.h"
#include "../shader/PbrShader.h"
#include "../shader/ShadowShader.h"
#include "../shader/BlinnPhongWithShadowShader.h"
#include "../shader/PbrShader2.h"
#include "../shader/ToonShader.h"
#include "../shader/PointShader.h"
#include "../shader/RGBSpliterShader.h"
#include "../shader/GrayScaleShader.h"
#include "../shader/MosaicShader.h"
#include "../shader/NormalMap.h"
#include "../shader/FlatShader.h"
#include "../shader/DepthMap.h"

Renderer::Renderer() { }

void Renderer::SetDrawHandle(Window* w)
{
	winHandle = w;
}

void Renderer::InitShaders()
{
	shaderLibs.insert({ ShaderType::Gouraud, std::make_shared< GouraudShader>() });
	shaderLibs.insert({ ShaderType::Toon, std::make_shared<ToonShader>() });
	shaderLibs.insert({ ShaderType::Pixel, std::make_shared<PixelShader>() });
	shaderLibs.insert({ ShaderType::WireFrame, std::make_shared<WireFrameShader>(0.02) });
	shaderLibs.insert({ ShaderType::Point, std::make_shared<PointShader>() });
	shaderLibs.insert({ ShaderType::BlinnPhong, std::make_shared<BlinnPhongShader>() });
	shaderLibs.insert({ ShaderType::PBR, std::make_shared<PbrShader>() });
	shaderLibs.insert({ ShaderType::PBR2, std::make_shared<PbrShader2>() });
	shaderLibs.insert({ ShaderType::Shadow, std::make_shared<ShadowShader>() });
	shaderLibs.insert({ ShaderType::BlinnPhongWithShadow, std::make_shared<BlinnPhongWithShadowShader>() });
	shaderLibs.insert({ ShaderType::RGBSpliter, std::make_shared<RGBSpliterShader>(glm::vec2(10.0f, 10.0f)) });
	shaderLibs.insert({ ShaderType::GrayScale, std::make_shared<GrayScaleShader>() });
	shaderLibs.insert({ ShaderType::MosaicArt, std::make_shared<MosaicShader>(90.0f) });
	shaderLibs.insert({ ShaderType::NormalMap, std::make_shared<NormalMapShader>() });
	shaderLibs.insert({ ShaderType::Flat, std::make_shared<FlatShader>() });
	shaderLibs.insert({ ShaderType::DepthMap, std::make_shared<DepthShader>() });
}

void Renderer::BindShader(ShaderType type, ShaderStageType stage)
{
	auto res = shaderLibs.find(type);
	if (res != shaderLibs.cend())
	{
		auto curShader = res->second;
		if (stage == ShaderStageType::defaultRaster)
		{
			activeShader = curShader;
		}
		else
		{
			postProcessShader = curShader;
		}
	}
}

void Renderer::BeginScene(const std::shared_ptr<Scene>& scene)
{
	activeScene = scene;
}

void Renderer::EndScene()
{
	activeScene.reset();
}

void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	viewport.width  = width;
	viewport.height = height;
	viewport.transform = {
		{ width / 2, 0, 0, 0 },
		{ 0, height / 2, 0, 0 },
		{ 0, 0, 1, 0 },
		{ width / 2 + x, height / 2 + y, 0, 1 }
	};
	defaultPassFrameBuffer = std::make_unique<FrameBuffer>(width, height);
	shadowPassFrameBuffer = std::make_unique<FrameBuffer>(width, height);
	postProcessBuffer = std::make_shared < FrameBuffer>(width, height);
}

void Renderer::ShadowPass()
{
	BindShader(ShaderType::Shadow, ShaderStageType::defaultRaster);
	activeShader->light = activeScene->GetLight();
	activeShader->model = glm::identity<glm::mat4>();
	activeShader->itModel = glm::transpose(glm::inverse(activeShader->model));
	activeShader->viewProjection = activeScene->GetLight()->GetLightSpace().ViewProjection();
	glm::vec4 clipCoords[3];
	glm::vec3 localCoords[3];
	glm::vec2 uvs[3];
	if (activeScene != nullptr)
	{
		const std::vector<Model2>& models = activeScene->GetModels();
		size_t modelCount = models.size();
		for (size_t i = 0; i < modelCount; i++)
		{
			const Model2& model = models[i];
			const std::vector<Primative> primatives = model.GetAllPrimatives();
			for (size_t p = 0; p < primatives.size(); p++)
			{
				const Primative& primative = primatives[p];
				size_t nface = primative.getNface();
				for (size_t j = 0; j < nface; j++)
				{
					for (size_t k = 0; k < 3; k++)
					{
						const glm::vec3& pos = primative.GetPosition(j, k);
						const glm::vec3& normal = primative.GetNormal(j, k);
						const glm::vec2& uv = primative.GetUV(j, k);
						localCoords[k] = pos;
						uvs[k] = uv;
						VertexAttribute vertex{ pos, normal, uv };
						activeShader->Vertex(clipCoords[k], vertex, (uint32_t)k);
					}
					Rasterize(clipCoords, shadowPassFrameBuffer, false);
				}
			}			
		}
	}
}

void Renderer::DefaultPass()
{
	activeShader->light = activeScene->GetLight();
	activeShader->pointLights = activeScene->GetPointLights();
	activeShader->model = glm::identity<glm::mat4>();
	activeShader->itModel = glm::mat3(glm::transpose(glm::inverse(activeShader->model)));
	activeShader->viewProjection = activeScene->GetCamera()->ViewProjection();
	activeShader->camPos = activeScene->GetCamera()->Position();
	activeShader->shadowBuffer = shadowPassFrameBuffer;
	activeShader->viewport = viewport.transform;
	glm::vec4 clipCoords[3];
	glm::vec3 localPosition[3];
	glm::vec2 uvs[3];
	if (activeScene != nullptr)
	{
		const std::vector<Model2>& models = activeScene->GetModels();
		size_t modelCount = models.size();
		for (size_t i = 0; i < modelCount; i++)
		{
			const Model2& model = models[i];
			const std::vector<Primative>& primatives = model.GetAllPrimatives();
			size_t primativeCount = primatives.size();
			for (size_t p = 0; p < primativeCount; p++)
			{
				const Primative& primative = primatives[p];
				size_t nface = primative.getNface();
				activeShader->SetSampler(0, primative.GetMaterial()->diffuse);
				activeShader->SetSampler(1, primative.GetMaterial()->specular);
				activeShader->SetSampler(2, primative.GetMaterial()->normal);
				activeShader->SetSampler(3, primative.GetMaterial()->ao);
				activeShader->SetSampler(4, primative.GetMaterial()->roughness);
				activeShader->SetSampler(5, primative.GetMaterial()->metalness);
				for (size_t j = 0; j < nface; j++)
				{
					for (size_t k = 0; k < 3; k++)
					{
						glm::vec3 position = primative.GetPosition(j, k);
						glm::vec3 normal = primative.GetNormal(j, k);
						glm::vec2 uv = primative.GetUV(j, k);
						localPosition[k] = position;
						uvs[k] = uv;
						activeShader->Vertex(clipCoords[k], { position, normal, uv }, (uint32_t)k);
					}
					ComputeTBN(localPosition, uvs);
					Rasterize(clipCoords, defaultPassFrameBuffer, !postProcessShader);
				}
			}			
		}
		//output image
		defaultPassFrameBuffer->colorAttachment.flip_vertically();
		defaultPassFrameBuffer->colorAttachment.write_tga_file("output.tga");
	}
}

void Renderer::PostProcess()
{
	//屏幕空间对前一個pass的attachment采样
	
	postProcessShader->prePassColorAttachment = defaultPassFrameBuffer->colorAttachment;
	uint32_t height = postProcessShader->prePassColorAttachment.get_height();
	uint32_t width = postProcessShader->prePassColorAttachment.get_width();
	glm::vec4 gl_FragColor;
	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			postProcessShader->currentPixel = glm::vec2(x, y);
			postProcessShader->Fragment(gl_FragColor);
			winHandle->DrawPoint(x, y, glm::vec3(gl_FragColor) * 255.0f, false);
			postProcessBuffer->colorAttachment.set(x, y, TGAColor((uint8_t)(gl_FragColor.r * 255), (uint8_t)(gl_FragColor.g * 255), (uint8_t)(gl_FragColor.b * 255), 255));
		}
	}
	//output image
	postProcessBuffer->colorAttachment.write_tga_file("postProcess.tga");
}

void Renderer::Draw()
{
	//ShadowPass(winHandle);
	BindShader(ShaderType::Toon, ShaderStageType::defaultRaster);
	//BindShader(ShaderType::RGBSpliter, ShaderStageType::postProcess);
	DefaultPass();
	if (postProcessShader)
	{
		PostProcess();
	}
}

void Renderer::DrawLine()
{
	if (activeScene != nullptr)
	{
		const std::vector<Model2>& models = activeScene->GetModels();
		size_t modelCount = models.size();
		for (size_t i = 0; i < modelCount; i++)
		{
			const Model2& model = models[i];
			const std::vector<Primative> primatives = model.GetAllPrimatives();
			size_t primativeCount = primatives.size();
			for (size_t p = 0; p < primativeCount; p++)
			{
				const Primative& primative = primatives[p];
				size_t nface = primative.getNface();
				for (size_t j = 0; j < nface; j++)
				{
					for (uint32_t k = 0; k < 3; k++)
					{
						const glm::vec3& v0 = primative.GetPosition(j, k);
						const glm::vec3& v1 = primative.GetPosition(j, (k + 1) % 3);
						int x0 = (int)((v0.x + 1.0f) * viewport.width / 2.0f);
						int y0 = (int)((v0.y + 1.0f) * viewport.height / 2.0f);
						int x1 = (int)((v1.x + 1.0f) * viewport.width / 2.0f);
						int y1 = (int)((v1.y + 1.0f) * viewport.height / 2.0f);
						BresenhamLine(x0, y0, x1, y1, { 255, 255, 255 }, winHandle);
					}
				}
			}
		}
	}
}

void Renderer::Clear()
{
	defaultPassFrameBuffer->Reset();
	shadowPassFrameBuffer->Reset();
	postProcessBuffer->Reset();
}

void Renderer::Rasterize(glm::vec4* vertices, std::shared_ptr<FrameBuffer>& currentBuffer, bool present)
{
	glm::vec4 beforeClippedCoords[3] = { viewport.transform * vertices[0], viewport.transform * vertices[1], viewport.transform * vertices[2] };
	glm::vec4 afterClippedCoords[3] = { beforeClippedCoords[0] / beforeClippedCoords[0].w, beforeClippedCoords[1] / beforeClippedCoords[1].w,beforeClippedCoords[2] / beforeClippedCoords[2].w };
	BoundingBox bbox = GetBoundingBox(afterClippedCoords);
	for (uint32_t y = (uint32_t)bbox.min.y; y <= (uint32_t)bbox.max.y; y++)
	{
		for (uint32_t x = (uint32_t)bbox.min.x; x <= (uint32_t)bbox.max.x; x++)
		{
			glm::vec2 p = { x + 0.5f, y + 0.5f };
			glm::vec3 clipped_bc = BaryCentric(afterClippedCoords, p);
			if (clipped_bc.x < 0 || clipped_bc.y < 0 || clipped_bc.z < 0)
			{
				continue;
			}
			// 1/z = u/z0 + v/z1 + w/z2;
			float depth = 1.0f / (clipped_bc.x / beforeClippedCoords[0].z + clipped_bc.y / beforeClippedCoords[1].z + clipped_bc.z / beforeClippedCoords[2].z);

			uint32_t pixelIndex = x + y * viewport.width;
			if (depth < currentBuffer->zBuffer[pixelIndex])
			{
				activeShader->baryCentric = { depth * clipped_bc.x / beforeClippedCoords[0].z, depth * clipped_bc.y / beforeClippedCoords[1].z, depth * clipped_bc.z / beforeClippedCoords[2].z };
				glm::vec4 gl_FragColor;
				if (!activeShader->Fragment(gl_FragColor))
				{
					if (present)
					{
						winHandle->DrawPoint((int)p.x, (int)p.y, glm::vec3(gl_FragColor) * 255.0f);

					}
					currentBuffer->colorAttachment.set((int)p.x, (int)p.y, TGAColor((uint8_t)(gl_FragColor.r * 255), (uint8_t)(gl_FragColor.g * 255), (uint8_t)(gl_FragColor.b * 255), 255));
					currentBuffer->zBuffer[pixelIndex] = depth;
				}
			}
		}
	}
}

BoundingBox Renderer::GetBoundingBox(const glm::vec4* vertices)
{
	BoundingBox bbox;
	glm::vec2 min { 0, 0 };
	glm::vec2 max { viewport.width - 1, viewport.height - 1 };
	bbox.min = max;
	bbox.max = min;
	for (uint32_t i = 0; i < 3; i++)
	{
		bbox.min.x = glm::max(min.x, glm::min(bbox.min.x, vertices[i].x));
		bbox.min.y = glm::max(min.y, glm::min(bbox.min.y, vertices[i].y));
		bbox.max.x = glm::min(max.x, glm::max(bbox.max.x, vertices[i].x));
		bbox.max.y = glm::min(max.y, glm::max(bbox.max.y, vertices[i].y));
	}
	return bbox;
}

glm::vec3 Renderer::BaryCentric(const glm::vec4* vertices, const glm::vec2& p)
{
	glm::vec2 v0 = { vertices[0].x, vertices[0].y };
	glm::vec2 v1 = { vertices[1].x, vertices[1].y };
	glm::vec2 v2 = { vertices[2].x, vertices[2].y };
	
	/*
	 * u + v + w = 1;
	 * p = v0 + (v1-v0) * v + (v2-v0) * w ->
	 * (v1-v0) * v + (v2- v0) * w + (v0 -p) * 1 = 0
	 */
	glm::vec2 v01 = v1 - v0;
	glm::vec2 v02 = v2 - v0;
	glm::vec2 vp0 = v0 - p;
	glm::vec3 uvw = glm::cross(glm::vec3(v01.x, v02.x, vp0.x), glm::vec3(v01.y, v02.y, vp0.y));
	if (uvw.z < 1e-2)
	{
		return { -1, 1, 1 };
	}
	return { 1.0f - (uvw.x + uvw.y) / uvw.z, uvw.x / uvw.z, uvw.y / uvw.z };
}

void Renderer::ComputeTBN(glm::vec3* positions, glm::vec2* uvs)
{
	glm::vec3 e1 = positions[1] - positions[0];
	glm::vec3 e2 = positions[2] - positions[0];
	glm::vec2 deltaUV1 = uvs[1] - uvs[0];
	glm::vec2 deltaUV2 = uvs[2] - uvs[0];
	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * e1.x - deltaUV1.y * e2.x);
	tangent.y = f * (deltaUV2.y * e1.y - deltaUV1.y * e2.y);
	tangent.z = f * (deltaUV2.y * e1.z - deltaUV1.y * e2.z);
	tangent = glm::normalize(tangent);

	glm::vec3 bitangent;
	bitangent.x = f * (-deltaUV2.x * e1.x + deltaUV1.x * e2.x);
	bitangent.y = f * (-deltaUV2.x * e1.y + deltaUV1.x * e2.y);
	bitangent.z = f * (-deltaUV2.x * e1.z + deltaUV1.x * e2.z);
	bitangent = glm::normalize(bitangent);
	activeShader->tangent = tangent;
	activeShader->biTangent = bitangent;
}

void Renderer::BresenhamLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const glm::vec3& color, Window* winHandle)
{
	bool steep = false;
	if (abs(static_cast<int>(x1 - x0)) < abs(static_cast<int>(y1 - y0)))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (uint32_t x = x0; x <= x1; x++)
	{
		if (steep)
		{
			winHandle->DrawPoint(y, x, color);
		}
		else
		{
			winHandle->DrawPoint(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}