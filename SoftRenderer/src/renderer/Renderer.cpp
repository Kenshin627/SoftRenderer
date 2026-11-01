#include "Renderer.h"
#include "Triangle.h"
#include "../window/Window.h"
#include <random>
#include <iostream>
#include <limits>
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
#include "../shader/ToonEdgeShader.h"

Renderer::Renderer() { }

void Renderer::SetDrawHandle(Window* w)
{
	winHandle = w;
}

void Renderer::InitShaders()
{
	shaderLibs.insert({ ShaderType::Gouraud, std::make_shared< GouraudShader>() });
	shaderLibs.insert({ ShaderType::Toon, std::make_shared<ToonShader>() });
	shaderLibs[ShaderType::Toon]->baseColor = { 0.9f, 0.6f, 0.2f }; // solid toon color like the tutorial chapter image
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
	shaderLibs.insert({ ShaderType::ToonEdge, std::make_shared<ToonEdgeShader>(0.02f) });
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
		{ width / 2.0f, 0, 0, 0 },
		{ 0, height / 2.0f, 0, 0 },
		{ 0, 0, 1, 0 },
		{ width / 2.0f + x, height / 2.0f + y, 0, 1 }
	};
	//the shadow map runs at higher resolution than the camera framebuffer
	//(tutorial reference: 8000x8000 shadow buffer for an 800x800 output)
	shadowViewport.Set(0, 0, width * 4, height * 4);
	defaultPassFrameBuffer = std::make_unique<FrameBuffer>(width, height);
	defaultPassFrameBuffer->clearColor = { 209, 195, 177, 255 }; // light backdrop like the tutorial renders
	shadowPassFrameBuffer = std::make_unique<FrameBuffer>(width * 4, height * 4);
	postProcessBuffer = std::make_shared < FrameBuffer>(width, height);
}

void Renderer::ShadowPass()
{
	BindShader(ShaderType::Shadow, ShaderStageType::defaultRaster);
	activeShader->light = activeScene->GetLight();
	activeShader->model = glm::identity<glm::mat4>();
	activeShader->itModel = glm::transpose(glm::inverse(activeShader->model));
	activeShader->viewProjection = activeScene->GetLight()->GetLightSpace().ViewProjection();
	RenderSceneDepth(activeShader->viewProjection, shadowPassFrameBuffer);
}

void Renderer::RenderSceneDepth(const glm::mat4& viewProjection, std::shared_ptr<FrameBuffer>& target)
{
	if (activeScene == nullptr)
	{
		return;
	}
	glm::vec4 clipCoords[3];
	const std::vector<Model2>& models = activeScene->GetModels();
	size_t modelCount = models.size();
	for (size_t i = 0; i < modelCount; i++)
	{
		const Model2& model = models[i];
		const std::vector<Primative>& primatives = model.GetAllPrimatives();
		for (size_t p = 0; p < primatives.size(); p++)
		{
			const Primative& primative = primatives[p];
			size_t nface = primative.getNface();
			for (size_t j = 0; j < nface; j++)
			{
				for (size_t k = 0; k < 3; k++)
				{
					VertexAttribute vertex{ primative.GetPosition(j, k), primative.GetNormal(j, k), primative.GetUV(j, k) };
					activeShader->Vertex(clipCoords[k], vertex, (uint32_t)k);
				}
				Rasterize(clipCoords, target, false);
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
	activeShader->shadowViewport = shadowViewport.transform;
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
					glm::vec3 normals[3];
					for (size_t k = 0; k < 3; k++)
					{
						localPosition[k] = primative.GetPosition(j, k);
						normals[k] = primative.GetNormal(j, k);
						uvs[k] = primative.GetUV(j, k);
					}
					// per-face tangent frame (t b) = U^-1 * E (tutorial tangent chapter);
					// face corners are duplicated per triangle, so all three corners share
					// it and the barycentric interpolation carries it like any other varying
					glm::vec3 e1 = localPosition[1] - localPosition[0];
					glm::vec3 e2 = localPosition[2] - localPosition[0];
					glm::vec2 d1 = uvs[1] - uvs[0];
					glm::vec2 d2 = uvs[2] - uvs[0];
					float uvDet = d1.x * d2.y - d2.x * d1.y;
					glm::vec3 tangent = e1, bitangent = e2;
					if (std::abs(uvDet) > 1e-8f)
					{
						tangent = (e1 * d2.y - e2 * d1.y) / uvDet;
						bitangent = (e2 * d1.x - e1 * d2.x) / uvDet;
					}
					for (size_t k = 0; k < 3; k++)
					{
						VertexAttribute vertex{ localPosition[k], normals[k], uvs[k], tangent, bitangent };
						activeShader->Vertex(clipCoords[k], vertex, (uint32_t)k);
					}
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
	//the color attachment is already flip_vertically()'d (top-down) while the z buffer keeps
	//raster orientation (bottom-up): row-flip the depth copy to align with the color
	{
		uint32_t fw = defaultPassFrameBuffer->width;
		uint32_t fh = defaultPassFrameBuffer->height;
		postProcessShader->prePassDepth.resize(fw * fh);
		for (uint32_t y = 0; y < fh; y++)
		{
			for (uint32_t x = 0; x < fw; x++)
			{
				postProcessShader->prePassDepth[x + y * fw] = defaultPassFrameBuffer->zBuffer[x + (fh - 1 - y) * fw];
			}
		}
	}
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
	ShadowPass();
	//toon verification (tutorial toon chapter): quantized shading + sobel depth outline
	BindShader(ShaderType::Toon, ShaderStageType::defaultRaster);
	BindShader(ShaderType::ToonEdge, ShaderStageType::postProcess);
	//BindShader(ShaderType::BlinnPhongWithShadow, ShaderStageType::defaultRaster);
	//BindShader(ShaderType::RGBSpliter, ShaderStageType::postProcess);
	DefaultPass();
	//the scene is static: compute the (expensive) ao once from the first rendered
	//frame's depth buffer, then apply it per pixel in subsequent frames
	if (aoMode != AoMode::Off && !aoComputed)
	{
		std::cout << "computing ambient occlusion (one-shot for the static scene)..." << std::endl;
		if (aoMode == AoMode::SSAO)
		{
			ComputeSsao();
		}
		else
		{
			ComputeBruteForceAo();
		}
		aoComputed = true;
	}
	if (postProcessShader)
	{
		PostProcess();
	}
}

void Renderer::SetAoMode(AoMode mode)
{
	aoMode = mode;
	aoComputed = false;
	aoBuffer.clear();
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

void Renderer::ComputeSsao()
{
	//tutorial ao chapter, homework 2: for each fragment, sample points in a hemisphere
	//oriented along the surface normal, project them back to the screen and compare
	//against the depth buffer; average the occlusion votes. The reference branch samples
	//a ball directly in NDC space, which only works for its f-parameterized perspective;
	//our glm perspective warps NDC z too much for that, so we sample in world space
	//around the reconstructed fragment position, as the chapter text describes.
	constexpr float aoRadius = 0.15f;     // hemisphere radius in world units
	constexpr uint32_t sampleCount = 128; // samples in the hemisphere
	constexpr float depthBias = 2e-4f;    // NDC bias against self-occlusion acne
	constexpr float rangeCheck = 0.01f;   // NDC range check: skip much-closer false occluders
	constexpr float maxDarkening = 0.4f;

	FrameBuffer& fb = *defaultPassFrameBuffer;
	const uint32_t w = fb.width;
	const uint32_t h = fb.height;
	glm::mat4 viewProjection = activeScene->GetCamera()->ViewProjection();
	glm::mat4 invViewProjection = glm::inverse(viewProjection);
	aoBuffer.assign(w * h, 1.0f);
	std::mt19937 rng(12345u);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	auto smoothstep = [](float e0, float e1, float x)
	{
		float t = glm::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	};

	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			uint32_t idx = x + y * w;
			float z = fb.zBuffer[idx];
			if (z == std::numeric_limits<float>::max()) continue; // background
			//reconstruct the world position and the world normal of this fragment
			glm::vec4 ndc = { 2.0f * (x + 0.5f) / w - 1.0f, 2.0f * (y + 0.5f) / h - 1.0f, z, 1.0f };
			glm::vec4 worldPos = invViewProjection * ndc;
			worldPos /= worldPos.w;
			TGAColor nc = fb.normalAttachment.get((int)x, (int)y);
			glm::vec3 normal = glm::normalize((glm::vec3(nc.r, nc.g, nc.b) / 255.0f) * 2.0f - 1.0f);
			//tangent frame to orient the sampling hemisphere along the normal
			glm::vec3 tangent = glm::normalize(glm::cross(glm::abs(normal.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), normal));
			glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));
			float vote = 0.0f;
			float voters = 0.0f;
			for (uint32_t s = 0; s < sampleCount; s++)
			{
				//uniform sample on the hemisphere around the normal
				float phi = 2.0f * 3.14159265358979f * unit(rng);
				float cosTheta = unit(rng);
				float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);
				glm::vec3 dir = tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + normal * cosTheta;
				glm::vec3 samplePos = glm::vec3(worldPos) + dir * aoRadius;
				glm::vec4 q = viewProjection * glm::vec4(samplePos, 1.0f);
				q /= q.w;
				if (q.x < -1.0f || q.x >= 1.0f || q.y < -1.0f || q.y >= 1.0f) continue;
				uint32_t sx = (uint32_t)((q.x + 1.0f) * 0.5f * w);
				uint32_t sy = (uint32_t)((q.y + 1.0f) * 0.5f * h);
				float d = fb.zBuffer[sx + sy * w];
				if (d == std::numeric_limits<float>::max()) continue;   // sample lands on the background
				if (z - rangeCheck > d) continue;                       // much-closer surface: silhouette false occluder
				voters++;
				if (d + depthBias < q.z) vote++;                        // surface closer than the sample => occluded
			}
			float occlusion = voters > 0.0f ? vote / voters : 0.0f;
			aoBuffer[idx] = smoothstep(0.0f, 1.0f, 1.0f - occlusion * maxDarkening);
		}
	}

	//the chapter asks how to reduce the noise: a couple of box blur passes over the ao map
	auto boxBlur = [w, h](const std::vector<float>& src, std::vector<float>& dst)
	{
		for (uint32_t y = 0; y < h; y++)
		{
			for (uint32_t x = 0; x < w; x++)
			{
				float sum = 0.0f;
				int n = 0;
				for (int dy = -1; dy <= 1; dy++)
				{
					for (int dx = -1; dx <= 1; dx++)
					{
						int nx = (int)x + dx;
						int ny = (int)y + dy;
						if (nx < 0 || nx >= (int)w || ny < 0 || ny >= (int)h) continue;
						sum += src[nx + ny * w];
						n++;
					}
				}
				dst[x + y * w] = sum / n;
			}
		}
	};
	std::vector<float> tmp(w * h);
	boxBlur(aoBuffer, tmp);
	boxBlur(tmp, aoBuffer);

	//debug dump: ao statistics and grayscale map
	{
		float mn = 1.0f, mx = 0.0f, sum = 0.0f;
		uint32_t n = 0;
		for (uint32_t i = 0; i < w * h; i++)
		{
			if (fb.zBuffer[i] == std::numeric_limits<float>::max()) continue;
			mn = glm::min(mn, aoBuffer[i]);
			mx = glm::max(mx, aoBuffer[i]);
			sum += aoBuffer[i];
			n++;
		}
		std::cout << "ssao stats: n=" << n << " min=" << mn << " mean=" << (sum / n) << " max=" << mx << std::endl;
		TGAImage aoImg(w, h, TGAImage::GRAYSCALE);
		for (uint32_t y = 0; y < h; y++)
			for (uint32_t x = 0; x < w; x++)
			{
				uint8_t v = (uint8_t)(aoBuffer[x + y * w] * 255);
				aoImg.set((int)x, (int)y, TGAColor(v, 1));
			}
		aoImg.write_tga_file("ao_map.tga");
	}
}

void Renderer::ComputeBruteForceAo()
{
	//tutorial ao chapter, homework 1: average the binary visibility of many virtual
	//lights spread over the sphere (the chapter teaser averages ~1000 camera positions)
	constexpr uint32_t directionCount = 32;
	constexpr float depthBias = 5e-3f;
	const float PI = 3.14159265358979f;

	FrameBuffer& fb = *defaultPassFrameBuffer;
	const uint32_t w = fb.width;
	const uint32_t h = fb.height;
	const uint32_t aw = w / 2;
	const uint32_t ah = h / 2;
	if (!aoDepthBuffer || aoDepthBuffer->width != aw || aoDepthBuffer->height != ah)
	{
		aoDepthBuffer = std::make_shared<FrameBuffer>(aw, ah);
	}
	Viewport aoViewport;
	aoViewport.Set(0, 0, aw, ah);
	glm::mat4 invViewProjection = glm::inverse(activeScene->GetCamera()->ViewProjection());

	//fibonacci-sphere directions
	std::vector<glm::vec3> directions;
	for (uint32_t i = 0; i < directionCount; i++)
	{
		float t = (i + 0.5f) / directionCount;
		float phi = 2.0f * PI * i * (0.6180339887498949f * 2.0f); // golden angle spiral
		float cosTheta = 1.0f - 2.0f * t;
		float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		directions.push_back({ sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi) });
	}

	std::vector<float> visible(w * h, 0.0f);
	std::vector<float> valid(w * h, 0.0f);
	for (const glm::vec3& dir : directions)
	{
		//depth pass from this direction (half resolution ortho camera like the shadow pass)
		Camera aoCamera(-dir * 2.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		aoCamera.OrthoGraphic(-2.0f, 2.0f, -2.0f, 2.0f, 0.01f, 10.0f);
		aoDepthBuffer->Reset();
		BindShader(ShaderType::Shadow, ShaderStageType::defaultRaster);
		activeShader->model = glm::identity<glm::mat4>();
		activeShader->itModel = glm::transpose(glm::inverse(activeShader->model));
		activeShader->viewProjection = aoCamera.ViewProjection();
		RenderSceneDepth(aoCamera.ViewProjection(), aoDepthBuffer);
		glm::mat4 lightScreen = aoViewport.transform * aoCamera.ViewProjection();

		for (uint32_t y = 0; y < h; y++)
		{
			for (uint32_t x = 0; x < w; x++)
			{
				uint32_t idx = x + y * w;
				float z = fb.zBuffer[idx];
				if (z == std::numeric_limits<float>::max()) continue; // background
				//reconstruct the world position and normal of this fragment
				glm::vec4 ndc = { 2.0f * (x + 0.5f) / w - 1.0f, 2.0f * (y + 0.5f) / h - 1.0f, z, 1.0f };
				glm::vec4 worldPos = invViewProjection * ndc;
				worldPos /= worldPos.w;
				TGAColor nc = fb.normalAttachment.get((int)x, (int)y);
				glm::vec3 normal = glm::normalize((glm::vec3(nc.r, nc.g, nc.b) / 255.0f) * 2.0f - 1.0f);
				//skip directions the surface faces away from
				if (glm::dot(normal, -dir) <= 0.0f) continue;
				glm::vec4 q = lightScreen * worldPos;
				q /= q.w;
				valid[idx]++;
				if (q.x < 0.0f || q.x >= (float)aw || q.y < 0.0f || q.y >= (float)ah)
				{
					visible[idx] += 1.0f; // outside the depth map: unoccluded (same as the shadow shader)
					continue;
				}
				float d = aoDepthBuffer->zBuffer[(uint32_t)q.x + (uint32_t)q.y * aw];
				if (d + depthBias >= q.z) visible[idx] += 1.0f; // lit (same test as the shadow shader)
			}
		}
	}

	//average the directional visibility
	aoBuffer.assign(w * h, 1.0f);
	for (uint32_t i = 0; i < w * h; i++)
	{
		if (valid[i] > 0.0f) aoBuffer[i] = visible[i] / valid[i];
	}

	//debug dump: ao statistics and grayscale map
	{
		float mn = 1.0f, mx = 0.0f, sum = 0.0f;
		uint32_t n = 0;
		for (uint32_t i = 0; i < w * h; i++)
		{
			if (valid[i] <= 0.0f) continue;
			mn = glm::min(mn, aoBuffer[i]);
			mx = glm::max(mx, aoBuffer[i]);
			sum += aoBuffer[i];
			n++;
		}
		std::cout << "brute-force ao stats: n=" << n << " min=" << mn << " mean=" << (sum / n) << " max=" << mx << std::endl;
		TGAImage aoImg(w, h, TGAImage::GRAYSCALE);
		for (uint32_t y = 0; y < h; y++)
			for (uint32_t x = 0; x < w; x++)
			{
				uint8_t v = (uint8_t)(aoBuffer[x + y * w] * 255);
				aoImg.set((int)x, (int)y, TGAColor(v, 1));
			}
		aoImg.write_tga_file("ao_map.tga");
	}

	//smooth the 32-direction estimate a little (the reference averages ~1000 directions)
	auto boxBlur = [w, h](const std::vector<float>& src, std::vector<float>& dst)
	{
		for (uint32_t y = 0; y < h; y++)
		{
			for (uint32_t x = 0; x < w; x++)
			{
				float sum = 0.0f;
				int n = 0;
				for (int dy = -1; dy <= 1; dy++)
				{
					for (int dx = -1; dx <= 1; dx++)
					{
						int nx = (int)x + dx;
						int ny = (int)y + dy;
						if (nx < 0 || nx >= (int)w || ny < 0 || ny >= (int)h) continue;
						sum += src[nx + ny * w];
						n++;
					}
				}
				dst[x + y * w] = sum / n;
			}
		}
	};
	std::vector<float> tmp(w * h);
	boxBlur(aoBuffer, tmp);
	boxBlur(tmp, aoBuffer);
}

void Renderer::Clear()
{
	defaultPassFrameBuffer->Reset();
	shadowPassFrameBuffer->Reset();
	postProcessBuffer->Reset();
}

void Renderer::Rasterize(glm::vec4* vertices, std::shared_ptr<FrameBuffer>& currentBuffer, bool present)
{
	//the viewport transform is derived from the target buffer, since the shadow buffer
	//runs at a higher resolution than the camera framebuffer
	Viewport bufferViewport;
	bufferViewport.Set(0, 0, currentBuffer->width, currentBuffer->height);
	glm::vec4 beforeClippedCoords[3] = { bufferViewport.transform * vertices[0], bufferViewport.transform * vertices[1], bufferViewport.transform * vertices[2] };
	glm::vec4 afterClippedCoords[3] = { beforeClippedCoords[0] / beforeClippedCoords[0].w, beforeClippedCoords[1] / beforeClippedCoords[1].w,beforeClippedCoords[2] / beforeClippedCoords[2].w };

	// tutorial rasterization chapter: back-face culling + discard triangles covering less than one pixel.
	// signed doubled area via 2D cross product (same convention as BaryCentric):
	// counter-clockwise (front-facing) > 0, clockwise (back-facing) < 0, |area| < 1px => sub-pixel.
	float signedArea2 = (afterClippedCoords[1].x - afterClippedCoords[0].x) * (afterClippedCoords[2].y - afterClippedCoords[0].y)
	                  - (afterClippedCoords[2].x - afterClippedCoords[0].x) * (afterClippedCoords[1].y - afterClippedCoords[0].y);
	if (signedArea2 < 1.0f) return;

	BoundingBox bbox = GetBoundingBox(afterClippedCoords, currentBuffer->width, currentBuffer->height);
	for (uint32_t y = (uint32_t)bbox.min.y; y <= (uint32_t)bbox.max.y; y++)
	{
		for (uint32_t x = (uint32_t)bbox.min.x; x <= (uint32_t)bbox.max.x; x++)
		{
			glm::vec2 p = { x + 0.5f, y + 0.5f };
			glm::vec3 bc_screen = BaryCentric(afterClippedCoords, p);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
			{
				continue;
			}
			//tutorial our_gl rasterize: depth = screen-linear interpolation of the ndc z
			float depth = bc_screen.x * afterClippedCoords[0].z + bc_screen.y * afterClippedCoords[1].z + bc_screen.z * afterClippedCoords[2].z;

			uint32_t pixelIndex = x + y * currentBuffer->width;
			if (depth < currentBuffer->zBuffer[pixelIndex])
			{
				//tutorial our_gl rasterize: perspective-correct barycentric for varying
				//interpolation divides by the clip-space w (not z)
				glm::vec3 bc_clip = { bc_screen.x / beforeClippedCoords[0].w, bc_screen.y / beforeClippedCoords[1].w, bc_screen.z / beforeClippedCoords[2].w };
				bc_clip /= (bc_clip.x + bc_clip.y + bc_clip.z);
				activeShader->baryCentric = bc_clip;
				//per-pixel ambient occlusion multiplier (full-res buffer only)
				activeShader->aoFactor = (!aoBuffer.empty() && currentBuffer->width == viewport.width && currentBuffer->height == viewport.height) ? aoBuffer[pixelIndex] : 1.0f;
				glm::vec4 gl_FragColor;
				if (!activeShader->Fragment(gl_FragColor))
				{
					if (present)
					{
						winHandle->DrawPoint((int)p.x, (int)p.y, glm::vec3(gl_FragColor) * 255.0f);

					}
					//cache the interpolated world normal for the ao passes (only needed on the main buffer)
					if (currentBuffer.get() == defaultPassFrameBuffer.get())
					{
						glm::vec3 n = glm::normalize(bc_clip.x * activeShader->triangle.vertices[0].normal + bc_clip.y * activeShader->triangle.vertices[1].normal + bc_clip.z * activeShader->triangle.vertices[2].normal);
						currentBuffer->normalAttachment.set((int)p.x, (int)p.y, TGAColor((uint8_t)((n.x * 0.5f + 0.5f) * 255), (uint8_t)((n.y * 0.5f + 0.5f) * 255), (uint8_t)((n.z * 0.5f + 0.5f) * 255), 255));
					}
					currentBuffer->colorAttachment.set((int)p.x, (int)p.y, TGAColor((uint8_t)(gl_FragColor.r * 255), (uint8_t)(gl_FragColor.g * 255), (uint8_t)(gl_FragColor.b * 255), 255));
					currentBuffer->zBuffer[pixelIndex] = depth;
				}
			}
		}
	}
}

BoundingBox Renderer::GetBoundingBox(const glm::vec4* vertices, uint32_t width, uint32_t height)
{
	BoundingBox bbox;
	glm::vec2 min { 0, 0 };
	glm::vec2 max { width - 1.0f, height - 1.0f };
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