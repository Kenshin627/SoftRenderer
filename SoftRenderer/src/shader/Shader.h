#pragma once
#include "../renderer/Triangle.h"
#include "../light/DirectionLight.h"
#include "../light/PointLight.h"
#include "../renderer/FrameBuffer.h"
#include "../texture/Texture.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

enum ShaderType
{
	deActive = -1,
	Gouraud = 0,
	Pixel,
	WireFrame,
	Point,
	BlinnPhong,
	Toon,
	PBR,
	PBR2,
	Shadow,
	BlinnPhongWithShadow,
	RGBSpliter,
	GrayScale,
	MosaicArt,
	NormalMap,
	Flat,
	DepthMap,
	ToonEdge
};

enum class ShaderStageType
{
	defaultRaster,
	postProcess
};

class Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) = 0;
	virtual bool Fragment(glm::vec4& gl_fragColor) = 0;
	virtual void SetSampler(uint32_t slot, const std::shared_ptr<Texture>& texture);
	virtual glm::vec3 Sampler2D(const glm::vec2& uv, const std::shared_ptr<Texture>& texture);
public:
	Triangle triangle;
	glm::vec3 baryCentric{ 0.0f, 0.0f, 0.0f };
	//uniforms
	glm::mat4 model;
	glm::mat3 itModel;
	glm::mat4 viewProjection;
	std::shared_ptr<DirectionLight> light;
	std::vector<std::shared_ptr<PointLight>> pointLights;
	glm::vec3 camPos;
	//samplers
	std::shared_ptr<Texture> diffuseTexture; //slot 0
	std::shared_ptr<Texture> specularTexture;//slot 1
	std::shared_ptr<Texture> tbnNormalTexture;//slot 2
	std::shared_ptr<Texture> aoTexture;//slot 3
	std::shared_ptr<Texture> roughnessTexture;//slot 4
	std::shared_ptr<Texture> metalnessTexture;//slot 5
	std::shared_ptr<Texture> emissiveTexture;//slot 6 (_glow.tga)


	//baseColor
	glm::vec3 baseColor = { 1, 1, 1 };

	//shadowMap
	std::shared_ptr<FrameBuffer> shadowBuffer;
	glm::mat4 viewport;
	glm::mat4 shadowViewport; //viewport matching the (higher-res) shadow buffer

	//ambient occlusion: per-pixel multiplier filled by the renderer (1 = no occlusion)
	float aoFactor = 1.0f;

	//postProcess
	glm::vec2 currentPixel;
	TGAImage prePassColorAttachment;
	std::vector<float> prePassDepth; // depth buffer of the previous pass (for sobel outlines / ao)
};