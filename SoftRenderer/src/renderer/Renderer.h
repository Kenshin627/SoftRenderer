#pragma once
#include "../scene/Scene.h"
#include "../shader/Shader.h"
#include "FrameBuffer.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

struct Viewport
{
	Viewport() = default;
	void Set(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
	{
		width  = w;
		height = h;
		transform =
		{
			{ width / 2.0f,     0,              0, 0 },
			{ 0,             height / 2.0f,     0, 0 },
			{ 0,             0,              1, 0 },
			{ width / 2.0f + x, height / 2.0f + y, 0, 1 }
		};
	}
	uint32_t width = 0;
	uint32_t height = 0;
	glm::mat4 transform = glm::identity<glm::mat4>();
};

struct BoundingBox
{
	glm::vec2 min;
	glm::vec2 max;
};

class Renderer
{
public:
	//ambient occlusion modes (tutorial ao chapter: homework 1 = brute force average of
	//many directional shadow tests, homework 2 = screen-space ambient occlusion)
	enum class AoMode { Off, SSAO, BruteForce };

	Renderer();
	~Renderer() = default;
	void InitShaders();
	void BindShader(ShaderType type, ShaderStageType stage);
	void BeginScene(const std::shared_ptr<Scene>& scene);
	void EndScene();
	void Draw();
	void DrawLine();
	void ShadowPass();
	void DefaultPass();
	void PostProcess();
	void Clear();
	void Rasterize(const glm::vec4* vertices, std::shared_ptr<FrameBuffer>& currentBuffer, const glm::vec2* uvs = nullptr);
	BoundingBox GetBoundingBox(const glm::vec4* vertices, uint32_t width, uint32_t height);
	glm::vec3 BaryCentric(const glm::vec4* vertices, const glm::vec2& p);
	void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void SetAoMode(AoMode mode);
	static constexpr uint32_t MaxShadowSize = 8192;
	void SetShadowSize(uint32_t side); // 0 follows the camera viewport
	void SetShadowFilter(ShadowFilter value) { shadowFilter = value; }
	void SetTextureFilter(TextureFilter value);
	void SetAlpha(AlphaMode mode, float cutoff);
	void SetNormalSource(NormalSource source) { normalSource = source; }
	void SetHdr(const HdrSettings& settings) { hdr = settings; }
	void SetMsaa(bool enabled);
	void SetScanline(bool enabled) { scanline = enabled; }
	const TGAImage& Image() const;
	bool SavePasses() const;
	bool SaveAo(const char* path) const;
private:
	void BresenhamLine(glm::ivec2 a, glm::ivec2 b);
	void RasterizeTriangle(const glm::vec4* vertices, const glm::mat3& sourceWeights, std::shared_ptr<FrameBuffer>& target, const glm::vec2* uvs);
	void RenderSceneDepth(const glm::mat4& viewProjection, std::shared_ptr<FrameBuffer>& target);
	void BindMaterial(const Material& material);
	void DrawSurfaceFace(const Primative& mesh, size_t face);
	bool HasBlendedMaterial() const;
	void ComputeSsao();
	void BlurAo();
	void ComputeBruteForceAo();
	std::shared_ptr<Scene> activeScene;
	std::unordered_map<ShaderType, std::shared_ptr<Shader>> shaderLibs;
	std::shared_ptr<Shader> activeShader;
	std::shared_ptr<Shader> postProcessShader;
	Viewport viewport = Viewport();
	uint32_t shadowSize = 0;
	ShadowFilter shadowFilter = ShadowFilter::Hard;
	Viewport shadowViewport = Viewport();
	std::shared_ptr<FrameBuffer> defaultPassFrameBuffer;
	std::shared_ptr<FrameBuffer> shadowPassFrameBuffer;
	std::shared_ptr<FrameBuffer> postProcessBuffer;
	ShaderType surface = ShaderType::Toon;
	AlphaMode alphaOverride = AlphaMode::Auto;
	float alphaCutoff = 0.5f;
	const Material* activeMaterial = nullptr;
	AlphaMode activeAlpha = AlphaMode::Opaque;
	TextureFilter textureFilter = TextureFilter::Nearest;
	NormalSource normalSource = NormalSource::Auto;
	bool scanline = false;
	bool msaa = false;
	HdrSettings hdr;
	//ambient occlusion state (static scene: computed once, applied per pixel afterwards)
	AoMode aoMode = AoMode::Off;
	bool aoComputed = false;
	std::vector<float> aoBuffer;
	std::shared_ptr<FrameBuffer> aoDepthBuffer;
};