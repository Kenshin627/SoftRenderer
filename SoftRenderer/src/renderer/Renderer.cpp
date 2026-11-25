#include "Renderer.h"
#include <stdexcept>
#include "Triangle.h"
#include <algorithm>
#include <array>
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
	shaderLibs[ShaderType::Albedo] = std::make_shared<MaterialPreviewShader>(false);
	shaderLibs[ShaderType::Emission] = std::make_shared<MaterialPreviewShader>(true);
	shaderLibs[ShaderType::Lit] = shaderLibs.at(ShaderType::BlinnPhongWithShadow);
	shaderLibs[ShaderType::Phong] = std::make_shared<BlinnPhongWithShadowShader>(true);
	shaderLibs[ShaderType::BarycentricColor] = std::make_shared<BarycentricShader>();
	shaderLibs.insert({ ShaderType::Flat, std::make_shared<FlatShader>() });
	shaderLibs.insert({ ShaderType::DepthMap, std::make_shared<DepthShader>() });
	shaderLibs.insert({ ShaderType::ToonEdge, std::make_shared<ToonEdgeShader>(0.02f) });
}

void Renderer::BindShader(ShaderType type, ShaderStageType stage)
{
	if (stage == ShaderStageType::defaultRaster)
	{
		surface = type;
	}
	else
	{
		postProcessShader = type == ShaderType::deActive ? nullptr : shaderLibs.at(type);
	}
}

void Renderer::BeginScene(const std::shared_ptr<Scene>& scene)
{
	activeScene = scene;
	SetAoMode(aoMode);
}

void Renderer::EndScene()
{
	activeScene.reset();
}

void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	viewport.Set(x, y, width, height);
	defaultPassFrameBuffer = std::make_unique<FrameBuffer>(width, height);
	defaultPassFrameBuffer->clearColor = { 209, 195, 177, 255 }; // light backdrop like the tutorial renders
	shadowPassFrameBuffer.reset();
	SetAoMode(aoMode);
	postProcessBuffer = std::make_shared < FrameBuffer>(width, height);
}

void Renderer::SetShadowSize(uint32_t side)
{
	if (side > MaxShadowSize) throw std::invalid_argument("shadow map side exceeds 8192");
	shadowSize = side;
}

void Renderer::ShadowPass()
{
	uint32_t width = shadowSize ? shadowSize : viewport.width * 4;
	uint32_t height = shadowSize ? shadowSize : viewport.height * 4;
	shadowViewport.Set(0, 0, width, height);
	if (!shadowPassFrameBuffer || shadowPassFrameBuffer->width != width || shadowPassFrameBuffer->height != height)
		shadowPassFrameBuffer = std::make_shared<FrameBuffer>(width, height, true);
	else
		shadowPassFrameBuffer->Reset();
	RenderSceneDepth(activeScene->GetLight()->GetLightSpace().ViewProjection(), shadowPassFrameBuffer);
}

void Renderer::RenderSceneDepth(const glm::mat4& viewProjection, std::shared_ptr<FrameBuffer>& target)
{
	activeShader = shaderLibs.at(ShaderType::Shadow);
	activeShader->model = glm::mat4(1.0f);
	activeShader->viewProjection = viewProjection;
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
				Rasterize(clipCoords, target);
			}
		}
	}
}

void Renderer::DefaultPass()
{
	activeShader = shaderLibs.at(surface);
	activeShader->normalSource = normalSource;
	activeShader->textureFilter = textureFilter;
	activeShader->hdr = !defaultPassFrameBuffer->linearColor.empty();
	activeShader->emissionStrength = activeShader->hdr ? hdr.emissionStrength : 1.0f;
	const bool needsGradients = textureFilter == TextureFilter::Mipmap || textureFilter == TextureFilter::Trilinear;
	activeShader->light = activeScene->GetLight();
	activeShader->pointLights = activeScene->GetPointLights();
	activeShader->model = glm::identity<glm::mat4>();
	activeShader->itModel = glm::mat3(glm::transpose(glm::inverse(activeShader->model)));
	activeShader->viewProjection = activeScene->GetCamera()->ViewProjection();
	activeShader->camPos = activeScene->GetCamera()->Position();
	activeShader->shadowBuffer = surface == ShaderType::BlinnPhongWithShadow ? shadowPassFrameBuffer : nullptr;
	activeShader->viewport = viewport.transform;
	if (activeShader->shadowBuffer)
	{
		activeShader->worldToShadow = shadowViewport.transform * activeShader->light->GetLightSpace().ViewProjection();
		activeShader->shadowFilter = shadowFilter;
	}
	glm::vec4 clipCoords[3];
	glm::vec3 localPosition[3];
	glm::vec2 uvs[3];
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
			activeShader->objectNormalTexture = primative.GetMaterial()->objectNormal;
			activeShader->SetSampler(6, primative.GetMaterial()->emissive);
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
				glm::vec3 tangent(0.0f), bitangent(0.0f);
				if (std::abs(uvDet) > 1e-8f)
				{
					tangent = (e1 * d2.y - e2 * d1.y) / uvDet;
					// OBJ v was flipped for image addressing. Restore its sign for the tangent frame.
					bitangent = (e1 * d2.x - e2 * d1.x) / uvDet;
				}
				for (size_t k = 0; k < 3; k++)
				{
					VertexAttribute vertex{ localPosition[k], normals[k], uvs[k], tangent, bitangent };
					activeShader->Vertex(clipCoords[k], vertex, (uint32_t)k);
				}
				Rasterize(clipCoords, defaultPassFrameBuffer, needsGradients ? uvs : nullptr);
			}
		}
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
			postProcessBuffer->colorAttachment.set(x, y, TGAColor((uint8_t)(gl_FragColor.r * 255), (uint8_t)(gl_FragColor.g * 255), (uint8_t)(gl_FragColor.b * 255), 255));
		}
	}
}

const TGAImage& Renderer::Image() const
{
	return (postProcessShader ? postProcessBuffer : defaultPassFrameBuffer)->colorAttachment;
}

bool Renderer::SavePasses() const
{
	return defaultPassFrameBuffer->colorAttachment.write_tga_file("output.tga")
		&& (!postProcessShader || postProcessBuffer->colorAttachment.write_tga_file("postProcess.tga"));
}

bool Renderer::SaveAo(const char* path) const
{
	if (!aoComputed) return false;
	TGAImage image(viewport.width, viewport.height, TGAImage::GRAYSCALE);
	for (uint32_t y = 0; y < viewport.height; ++y)
		for (uint32_t x = 0; x < viewport.width; ++x)
			image.set(x, viewport.height - 1 - y, TGAColor(int(255 * aoBuffer[x + y * viewport.width]), 1));
	return image.write_tga_file(path);
}

void Renderer::Draw()
{
	if (!activeScene) return;
	const size_t pixels = size_t(viewport.width) * viewport.height;
	const bool multi = msaa && surface != ShaderType::Lines;
	const bool useHdr = hdr.enabled && surface != ShaderType::Lines && surface != ShaderType::DepthMap
		&& surface != ShaderType::NormalMap && surface != ShaderType::BarycentricColor;
	defaultPassFrameBuffer->samples.resize(multi ? pixels : 0);
	defaultPassFrameBuffer->linearColor.resize(useHdr ? pixels * (multi ? 4 : 1) : 0);
	Clear();
	if (surface == ShaderType::Lines)
	{
		DrawLine();
	}
	else
	{
		if (surface == ShaderType::BlinnPhongWithShadow) ShadowPass();
		DefaultPass();
		if (aoMode != AoMode::Off && !aoComputed)
		{
			if (aoMode == AoMode::SSAO) ComputeSsao();
			else ComputeBruteForceAo();
			aoComputed = true;
			defaultPassFrameBuffer->Reset();
			DefaultPass();
		}
	}
	if (useHdr) defaultPassFrameBuffer->FinishHdr(hdr);
	else defaultPassFrameBuffer->FinishColor(surface == ShaderType::DepthMap);
	defaultPassFrameBuffer->colorAttachment.flip_vertically();
	if (postProcessShader) PostProcess();
}

void Renderer::SetMsaa(bool enabled)
{
	if (msaa == enabled) return;
	msaa = enabled;
	SetAoMode(aoMode);
}

void Renderer::SetAoMode(AoMode mode)
{
	aoMode = mode;
	aoComputed = false;
	aoBuffer.clear();
}

namespace {
// 8 fractional bits keep shared-edge ties exact, including rotated sample positions.
constexpr int subpixelScale = 256;
const glm::ivec2 center(128, 128);
const std::array<glm::ivec2, 4> rotatedSamples = {{
	{96, 32}, {224, 96}, {32, 160}, {160, 224}
}};

// AO needs the position of the same surface sample as its resolved depth and normal.
glm::vec4 DepthNdc(const FrameBuffer& fb, uint32_t x, uint32_t y)
{
	size_t pixel = x + size_t(y) * fb.width;
	float z = fb.zBuffer[pixel];
	glm::vec2 offset(0.5f);
	if (!fb.samples.empty())
	{
		for (size_t i = 0; i < rotatedSamples.size(); ++i)
			if (fb.samples[pixel][i].depth == z)
			{
				offset = glm::vec2(rotatedSamples[i]) / float(subpixelScale);
				break;
			}
	}
	return {2.0f * (x + offset.x) / fb.width - 1.0f, 2.0f * (y + offset.y) / fb.height - 1.0f, z, 1.0f};
}

float planeDistance(const glm::vec4& p, int plane)
{
	return p.w + ((plane % 2 == 0) ? p[plane / 2] : -p[plane / 2]);
}
}

void Renderer::DrawLine()
{
	const auto vp = activeScene->GetCamera()->ViewProjection();
	for (const auto& model : activeScene->GetModels())
		for (const auto& mesh : model.GetAllPrimatives())
			for (size_t face = 0; face < mesh.getNface(); ++face)
				for (int edge = 0; edge < 3; ++edge)
				{
					auto a = vp * glm::vec4(mesh.GetPosition(face, edge), 1.0f);
					auto b = vp * glm::vec4(mesh.GetPosition(face, (edge + 1) % 3), 1.0f);
					bool visible = true;
					for (int plane = 0; plane < 6; ++plane)
					{
						float da = planeDistance(a, plane), db = planeDistance(b, plane);
						if (da < 0 && db < 0) { visible = false; break; }
						if (da < 0 || db < 0)
						{
							auto hit = glm::mix(a, b, da / (da - db));
							if (da < 0) a = hit; else b = hit;
						}
					}
					if (visible && a.w > 0 && b.w > 0)
						BresenhamLine(glm::ivec2(viewport.transform * (a / a.w)), glm::ivec2(viewport.transform * (b / b.w)));
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

	FrameBuffer& fb = *defaultPassFrameBuffer;
	const uint32_t w = fb.width;
	const uint32_t h = fb.height;
	glm::mat4 viewProjection = activeScene->GetCamera()->ViewProjection();
	glm::mat4 invViewProjection = glm::inverse(viewProjection);
	aoBuffer.assign(w * h, 1.0f);
	std::mt19937 rng(12345u);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);

	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			uint32_t idx = x + y * w;
			float z = fb.zBuffer[idx];
			if (z == std::numeric_limits<float>::max()) continue; // background
			//reconstruct the world position and the world normal of this fragment
			glm::vec4 ndc = DepthNdc(fb, x, y);
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
			aoBuffer[idx] = 1.0f - occlusion;
		}
	}

	//the chapter asks how to reduce the noise: a couple of box blur passes over the ao map
	BlurAo();
}

void Renderer::ComputeBruteForceAo()
{
	//tutorial ao chapter, homework 1: average the binary visibility of many virtual
	//lights spread over the sphere (the chapter teaser averages ~1000 camera positions)
	constexpr uint32_t directionCount = 256;
	constexpr float depthBias = 5e-3f;
	const float PI = 3.14159265358979f;

	FrameBuffer& fb = *defaultPassFrameBuffer;
	const uint32_t w = fb.width;
	const uint32_t h = fb.height;
	const uint32_t aw = glm::max(1u, w / 2);
	const uint32_t ah = h / 2;
	if (!aoDepthBuffer || aoDepthBuffer->width != aw || aoDepthBuffer->height != ah)
	{
		aoDepthBuffer = std::make_shared<FrameBuffer>(aw, ah, true);
	}
	Viewport aoViewport;
	aoViewport.Set(0, 0, aw, ah);
	glm::mat4 invViewProjection = glm::inverse(activeScene->GetCamera()->ViewProjection());

	//fibonacci-sphere directions
	std::vector<glm::vec3> directions;
	for (uint32_t i = 0; i < directionCount; i++)
	{
		float t = (i + 0.5f) / directionCount;
		float phi = 2.0f * PI * i * (0.6180339887498949f); // golden angle spiral
		float cosTheta = 1.0f - 2.0f * t;
		float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		directions.push_back({ sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi) });
	}

	std::vector<float> visible(w * h, 0.0f);
	std::vector<float> valid(w * h, 0.0f);
	for (const glm::vec3& dir : directions)
	{
		//depth pass from this direction (half resolution ortho camera like the shadow pass)
		Camera aoCamera(-dir * 4.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
		aoCamera.OrthoGraphic(-2.0f, 2.0f, -2.0f, 2.0f, 0.01f, 10.0f);
		aoDepthBuffer->Reset();
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
				glm::vec4 ndc = DepthNdc(fb, x, y);
				glm::vec4 worldPos = invViewProjection * ndc;
				worldPos /= worldPos.w;
				TGAColor nc = fb.normalAttachment.get((int)x, (int)y);
				glm::vec3 normal = glm::normalize((glm::vec3(nc.r, nc.g, nc.b) / 255.0f) * 2.0f - 1.0f);
				//skip directions the surface faces away from
				float cosine = glm::dot(normal, -dir);
				if (cosine <= 0.0f) continue;
				glm::vec4 q = lightScreen * worldPos;
				q /= q.w;
				valid[idx]++;
				if (q.x < 0.0f || q.x >= (float)aw || q.y < 0.0f || q.y >= (float)ah)
				{
					visible[idx] += 1.0f; // outside the depth map: unoccluded (same as the shadow shader)
					continue;
				}
				float d = aoDepthBuffer->zBuffer[(uint32_t)q.x + (uint32_t)q.y * aw];
				// Allow for the depth change across a half texel on a tilted surface.
				float slope = glm::sqrt(glm::max(0.0f, 1.0f - cosine * cosine)) / glm::max(cosine, 0.01f);
				float texel = 4.0f / glm::min(aw, ah);
				float bias = depthBias + (2.0f / (10.0f - 0.01f)) * texel * slope;
				if (d + bias >= q.z) visible[idx] += 1.0f; // lit (same test as the shadow shader)
			}
		}
	}

	//average the directional visibility
	aoBuffer.assign(w * h, 1.0f);
	for (uint32_t i = 0; i < w * h; i++)
	{
		if (valid[i] > 0.0f) aoBuffer[i] = visible[i] / valid[i];
	}

	//smooth the directional estimate a little (the reference averages ~1000 directions)
	BlurAo();
}

void Renderer::BlurAo()
{
	const auto w = viewport.width, h = viewport.height;
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
	postProcessBuffer->Reset();
}

void Renderer::Rasterize(const glm::vec4* vertices, std::shared_ptr<FrameBuffer>& target, const glm::vec2* uvs)
{
	// Clip before perspective division. Carry weights in the original triangle
	// so every shader keeps using its own three vertex outputs after clipping.
	struct Corner { glm::vec4 clip; glm::vec3 weights; };
	std::array<Corner, 9> first, second;
	Corner* polygon = first.data();
	Corner* next = second.data();
	const glm::mat3 identity(1.0f);
	int count = 3;
	for (int i = 0; i < count; ++i) polygon[i] = { vertices[i], identity[i] };
	for (int plane = 0; plane < 6; ++plane)
	{
		bool inside = true;
		for (int i = 0; i < count; ++i) inside &= planeDistance(polygon[i].clip, plane) >= 0;
		if (inside) continue;
		int output = 0;
		for (int i = 0; i < count; ++i)
		{
			const auto& a = polygon[(i + count - 1) % count];
			const auto& b = polygon[i];
			float da = planeDistance(a.clip, plane), db = planeDistance(b.clip, plane);
			if ((da < 0 && db > 0) || (da > 0 && db < 0))
			{
				// Evaluate shared clip intersections in the same direction for both faces.
				const auto& outside = da < 0 ? a : b;
				const auto& inside = da < 0 ? b : a;
				float t = da < 0 ? da / (da - db) : db / (db - da);
				next[output++] = { glm::mix(outside.clip, inside.clip, t), glm::mix(outside.weights, inside.weights, t) };
			}
			if (db >= 0) next[output++] = b;
		}
		if (output < 3) return;
		std::swap(polygon, next);
		count = output;
	}
	for (int i = 1; i + 1 < count; ++i)
	{
		glm::vec4 clip[] = { polygon[0].clip, polygon[i].clip, polygon[i+1].clip };
		glm::mat3 weights(polygon[0].weights, polygon[i].weights, polygon[i+1].weights);
		RasterizeTriangle(clip, weights, target, uvs);
	}
}

namespace {
struct CoverageEdge
{
	glm::ivec2 start, delta;
	int threshold; // A top or left edge accepts zero; other edges require a positive value.

	CoverageEdge(glm::ivec2 a, glm::ivec2 b) : start(a), delta(b - a),
		threshold((delta.y < 0 || (delta.y == 0 && delta.x < 0)) ? 0 : 1) {} // Y increases upwards

	int64_t At(glm::ivec2 p) const
	{
		return int64_t(delta.x) * (p.y - start.y) - int64_t(delta.y) * (p.x - start.x);
	}
};

// Bound all edge segments inside the row's sample band, including flat edges.
void ScanlineSpan(const glm::vec4* v, float bottom, float top, int& left, int& right)
{
	float lo = float(right + 1), hi = float(left - 1);
	for (int i = 0; i < 3; ++i)
	{
		glm::vec2 a(v[i]), b(v[(i + 1) % 3]);
		if (a.y > b.y) std::swap(a, b);
		if (a.y > top || b.y < bottom) continue;
		float x0 = a.x, x1 = b.x;
		if (a.y != b.y)
		{
			float slope = (b.x - a.x) / (b.y - a.y);
			x0 = a.x + (glm::max(a.y, bottom) - a.y) * slope;
			x1 = a.x + (glm::min(b.y, top) - a.y) * slope;
		}
		lo = glm::min(lo, glm::min(x0, x1));
		hi = glm::max(hi, glm::max(x0, x1));
	}
	left = glm::max(left, int(glm::floor(lo)) - 1);
	right = glm::min(right, int(glm::ceil(hi)));
}
}

void Renderer::RasterizeTriangle(const glm::vec4* vertices, const glm::mat3& sourceWeights, std::shared_ptr<FrameBuffer>& currentBuffer, const glm::vec2* uvs)
{
	// Each pass uses the dimensions of its own render target.
	Viewport bufferViewport;
	bufferViewport.Set(0, 0, currentBuffer->width, currentBuffer->height);
	glm::vec4 beforeClippedCoords[3] = { bufferViewport.transform * vertices[0], bufferViewport.transform * vertices[1], bufferViewport.transform * vertices[2] };
	glm::vec4 afterClippedCoords[3] = { beforeClippedCoords[0] / beforeClippedCoords[0].w, beforeClippedCoords[1] / beforeClippedCoords[1].w,beforeClippedCoords[2] / beforeClippedCoords[2].w };

	glm::ivec2 fixed[3];
	for (int i = 0; i < 3; ++i)
	{
		fixed[i] = glm::ivec2(glm::round(glm::vec2(afterClippedCoords[i]) * float(subpixelScale)));
		afterClippedCoords[i].x = float(fixed[i].x) / subpixelScale;
		afterClippedCoords[i].y = float(fixed[i].y) / subpixelScale;
	}
	const CoverageEdge edges[] = {{fixed[1], fixed[2]}, {fixed[2], fixed[0]}, {fixed[0], fixed[1]}};
	const int64_t area = edges[0].At(fixed[0]);
	if (area <= 0) return;
	const float invArea = 1.0f / float(area);
	const float signedArea2 = float(area) / (subpixelScale * subpixelScale);
	const bool multi = !currentBuffer->samples.empty();
	const int sampleCount = multi ? 4 : 1;
	const glm::ivec2* offsets = multi ? rotatedSamples.data() : &center;
	const bool depthOnly = currentBuffer != defaultPassFrameBuffer;
	const glm::vec3 ndcZ(afterClippedCoords[0].z, afterClippedCoords[1].z, afterClippedCoords[2].z);

	// H = (u/w, v/w, 1/w) varies linearly over the clipped screen triangle.
	glm::vec3 hDx(0.0f), hDy(0.0f);
	if (uvs)
	{
		glm::vec3 h[3];
		for (int i = 0; i < 3; ++i)
		{
			const auto& s = sourceWeights[i];
			const glm::vec2 uv = s.x * uvs[0] + s.y * uvs[1] + s.z * uvs[2];
			h[i] = glm::vec3(uv, 1.0f) / vertices[i].w;
		}
		const glm::vec2 e1(afterClippedCoords[1] - afterClippedCoords[0]);
		const glm::vec2 e2(afterClippedCoords[2] - afterClippedCoords[0]);
		hDx = ((h[1] - h[0]) * e2.y - (h[2] - h[0]) * e1.y) / signedArea2;
		hDy = ((h[2] - h[0]) * e1.x - (h[1] - h[0]) * e2.x) / signedArea2;
	}

	BoundingBox bbox = GetBoundingBox(afterClippedCoords, currentBuffer->width, currentBuffer->height);
	for (uint32_t y = (uint32_t)bbox.min.y; y <= (uint32_t)bbox.max.y; y++)
	{
		int left = int(bbox.min.x), right = int(bbox.max.x);
		if (scanline)
			ScanlineSpan(afterClippedCoords, y + (multi ? 0.125f : 0.5f), y + (multi ? 0.875f : 0.5f), left, right);

		for (int x = left; x <= right; ++x)
		{
			const size_t index = x + size_t(y) * currentBuffer->width;
			auto* quad = multi ? &currentBuffer->samples[index] : nullptr;
			glm::vec3 centroid(0.0f), closestWeights(0.0f);
			float depths[4], closest = currentBuffer->zBuffer[index];
			int covered = 0, visibleMask = 0;
			for (int i = 0; i < sampleCount; ++i)
			{
				glm::ivec2 point = glm::ivec2(x, y) * subpixelScale + offsets[i];
				const int64_t a = edges[0].At(point), b = edges[1].At(point), c = edges[2].At(point);
				if (a < edges[0].threshold || b < edges[1].threshold || c < edges[2].threshold) continue;
				glm::vec3 weights = glm::vec3(float(a), float(b), float(c)) * invArea;
				centroid += weights;
				++covered;
				depths[i] = glm::dot(weights, ndcZ);
				if (depths[i] >= (multi ? (*quad)[i].depth : currentBuffer->zBuffer[index])) continue;
				visibleMask |= 1 << i;
				if (depths[i] < closest) { closest = depths[i]; closestWeights = weights; }
			}
			if (!visibleMask) continue;
			if (depthOnly)
			{
				currentBuffer->zBuffer[index] = closest;
				continue;
			}

			// Shade once per primitive/pixel at the centroid of geometric coverage.
			// Occlusion by an earlier draw must not change the interpolation point.
			glm::vec3 bc = centroid / float(covered);
			const glm::vec3 clipW(vertices[0].w, vertices[1].w, vertices[2].w);
			bc /= clipW;
			const float reciprocalW = bc.x + bc.y + bc.z;
			bc = sourceWeights * (bc / reciprocalW);
			activeShader->baryCentric = bc;
			if (uvs)
			{
				const glm::vec2 uv = bc.x * uvs[0] + bc.y * uvs[1] + bc.z * uvs[2];
				activeShader->uvDx = (glm::vec2(hDx) - uv * hDx.z) / reciprocalW;
				activeShader->uvDy = (glm::vec2(hDy) - uv * hDy.z) / reciprocalW;
			}
			activeShader->aoFactor = aoBuffer.empty() ? 1.0f : aoBuffer[index];
			glm::vec4 fragment;
			if (activeShader->Fragment(fragment)) continue;
			currentBuffer->WriteFragment(x, y, index, visibleMask, depths, glm::vec3(fragment));

			if (closest < currentBuffer->zBuffer[index])
			{
				currentBuffer->zBuffer[index] = closest;
				if (aoMode != AoMode::Off && !aoComputed)
				{
					auto w = closestWeights / clipW;
					w = sourceWeights * (w / (w.x + w.y + w.z));
					const auto& v = activeShader->triangle.vertices;
					glm::vec3 n = glm::normalize(w.x * v[0].normal + w.y * v[1].normal + w.z * v[2].normal);
					n = (n * 0.5f + 0.5f) * 255.0f;
					currentBuffer->normalAttachment.set(x, y, TGAColor(uint8_t(n.x), uint8_t(n.y), uint8_t(n.z), 255));
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
	if (uvw.z <= 0.0f)
	{
		return { -1, 1, 1 };
	}
	return { 1.0f - (uvw.x + uvw.y) / uvw.z, uvw.x / uvw.z, uvw.y / uvw.z };
}

void Renderer::BresenhamLine(glm::ivec2 a, glm::ivec2 b)
{
	int dx = std::abs(b.x - a.x), dy = -std::abs(b.y - a.y);
	int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
	int error = dx + dy;
	for (;;)
	{
		defaultPassFrameBuffer->colorAttachment.set(a.x, a.y, TGAColor(38, 46, 56, 255));
		if (a == b) break;
		int twice = 2 * error;
		if (twice >= dy) { error += dy; a.x += sx; }
		if (twice <= dx) { error += dx; a.y += sy; }
	}
}
