#include "window/Window.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
const std::map<std::string, ShaderType> modes = {
	{"albedo", Albedo}, {"emission", Emission}, {"lit", Lit}, {"wire", Lines}, {"barycentric", BarycentricColor}, {"flat", Flat}, {"gouraud", Gouraud},
	{"smooth", Pixel}, {"phong", Phong}, {"blinn", BlinnPhong}, {"normal", NormalMap},
	{"shadow", BlinnPhongWithShadow}, {"toon", Toon}, {"depth", DepthMap}, {"pbr", PBR}
};
const std::map<std::string, ShaderType> effects = {
	{"none", deActive}, {"outline", ToonEdge}, {"gray", GrayScale}, {"rgb", RGBSpliter}, {"mosaic", MosaicArt}
};
const std::map<std::string, TextureFilter> filters = {
	{"nearest", TextureFilter::Nearest}, {"bilinear", TextureFilter::Bilinear},
	{"mipmap", TextureFilter::Mipmap}, {"trilinear", TextureFilter::Trilinear}
};
const std::map<std::string, AlphaMode> alphaModes = {
	{"auto", AlphaMode::Auto}, {"opaque", AlphaMode::Opaque}, {"mask", AlphaMode::Mask}, {"blend", AlphaMode::Blend}
};
const std::map<std::string, ShadowFilter> shadowFilters = {
	{"hard", ShadowFilter::Hard}, {"pcf", ShadowFilter::PCF}
};
const std::map<std::string, NormalSource> normals = {
	{"auto", NormalSource::Auto}, {"vertex", NormalSource::Vertex}, {"object", NormalSource::Object}, {"tangent", NormalSource::Tangent}
};
const std::map<std::string, Renderer::AoMode> occlusion = {
	{"off", Renderer::AoMode::Off}, {"ssao", Renderer::AoMode::SSAO}, {"brute", Renderer::AoMode::BruteForce}
};

template<class T>
T choice(const std::map<std::string, T>& choices, const std::string& value)
{
	auto found = choices.find(value);
	if (found == choices.end()) throw std::runtime_error("unknown choice: " + value);
	return found->second;
}

uint32_t parseShadowSize(const std::string& value)
{
	if (value == "auto") return 0;
	uint32_t side = 0;
	std::istringstream input(value);
	if (value.find_first_not_of("0123456789") != std::string::npos || !(input >> side) || side == 0 || side > Renderer::MaxShadowSize)
		throw std::runtime_error("--shadow-size expects auto or a side length from 1 to 8192");
	return side;
}

float number(const std::string& option, const std::string& value, float low = std::numeric_limits<float>::lowest(),
             float high = std::numeric_limits<float>::max())
{
	float result;
	std::istringstream input(value);
	if (!(input >> result) || !input.eof() || !std::isfinite(result))
		throw std::runtime_error(option + " expects a finite number");
	if (result < low || result > high)
		throw std::runtime_error(option + " expects a finite value in [" + std::to_string(low) + ", " + std::to_string(high) + "]");
	return result;
}

void usage()
{
	std::cout << "SoftRenderer [--scene diablo|head|helmet | --model file.obj ...] [--size WIDTHxHEIGHT] [--output frame.tga]\n"
		"  --mode albedo|emission|lit|wire|barycentric|flat|gouraud|smooth|phong|blinn|normal|shadow|toon|depth|pbr\n"
		"  --filter nearest|bilinear|mipmap|trilinear   (default: nearest)\n"
		"  --normals auto|vertex|object|tangent   --raster bbox|scanline   --msaa 1|4 (default: 1)\n"
		"  --hdr   --exposure EV (-16..16)   --emission-strength N (0..1024)   --bloom N (0..16)\n"
		"    HDR controls enable linear rendering. Defaults: EV 0, emission 1, bloom 0.\n"
		"  --env studio|panorama.hdr   --env-rotation DEGREES   --env-strength N (0..1024)\n"
		"    Environment lighting selects PBR and enables HDR. Defaults: rotation 0, strength 1.\n"
		"  --alpha auto|opaque|mask|blend   --alpha-cutoff N (0..1, default 0.5)\n"
		"  --shadow-size auto|N (1..8192)   --shadow-filter hard|pcf   (defaults: auto, hard)\n"
		"  --ao off|ssao|brute (phong/shadow)     --ao-map visibility.tga\n"
		"  --post none|outline|gray|rgb|mosaic   (toon defaults to outline)\n";
}
}

int main(int argc, char* argv[])
{
	try
	{
		std::string sceneName, output, aoOutput;
		std::vector<std::string> models;
		ShaderType mode = deActive, post = deActive;
		TextureFilter filter = TextureFilter::Nearest;
		ShadowFilter shadowFilter = ShadowFilter::Hard;
		uint32_t shadowSize = 0;
		AlphaMode alphaMode = AlphaMode::Auto;
		float alphaCutoff = 0.5f;
		HdrSettings hdr;
		std::string environmentPath;
		float environmentRotation = 0.0f, environmentStrength = 1.0f;
		bool environmentControl = false;
		NormalSource normal = NormalSource::Auto;
		auto ao = Renderer::AoMode::Off;
		bool explicitPost = false, scanline = false, msaa = false;
		int width = 1920, height = 1080;
		for (int i = 1; i < argc; ++i)
		{
			std::string key = argv[i];
			if (key == "--help") { usage(); return 0; }
			if (key == "--hdr") { hdr.enabled = true; continue; }
			if (++i == argc) throw std::runtime_error("missing value for " + key);
			std::string value = argv[i];
			if (key == "--scene") sceneName = value;
			else if (key == "--model") models.push_back(value);
			else if (key == "--mode") mode = choice(modes, value);
			else if (key == "--post") { post = choice(effects, value); explicitPost = true; }
			else if (key == "--filter") filter = choice(filters, value);
			else if (key == "--exposure" || key == "--emission-strength" || key == "--bloom")
			{
				hdr.enabled = true;
				if (key == "--exposure") hdr.exposureEv = number(key, value, -16, 16);
				else if (key == "--bloom") hdr.bloomStrength = number(key, value, 0, 16);
				else hdr.emissionStrength = number(key, value, 0, 1024);
			}
			else if (key == "--env")
			{
				if (value.empty()) throw std::runtime_error("--env expects studio or an HDR panorama path");
				environmentPath = value == "studio" ? "resource/environments/studio_small_09_1k.hdr" : value;
			}
			else if (key == "--env-rotation" || key == "--env-strength")
			{
				environmentControl = true;
				if (key == "--env-rotation")
					environmentRotation = number(key, value);
				else environmentStrength = number(key, value, 0.0f, 1024.0f);
			}
			else if (key == "--alpha") alphaMode = choice(alphaModes, value);
			else if (key == "--alpha-cutoff") alphaCutoff = number(key, value, 0.0f, 1.0f);
			else if (key == "--shadow-size") shadowSize = parseShadowSize(value);
			else if (key == "--shadow-filter") shadowFilter = choice(shadowFilters, value);
			else if (key == "--normals") normal = choice(normals, value);
			else if (key == "--ao") ao = choice(occlusion, value);
			else if (key == "--ao-map") aoOutput = value;
			else if (key == "--output") output = value;
			else if (key == "--msaa")
			{
				if (value != "1" && value != "4") throw std::runtime_error("--msaa expects 1 or 4");
				msaa = value == "4";
			}
			else if (key == "--raster")
			{
				if (value != "scanline" && value != "bbox") throw std::runtime_error("unknown rasterizer: " + value);
				scanline = value == "scanline";
			}
			else if (key == "--size")
			{
				char x; std::istringstream in(value);
				if (!(in >> width >> x >> height) || x != 'x' || !in.eof() ||
					width < 2 || height < 2 || width > 4096 || height > 4096)
					throw std::runtime_error("expected WIDTHxHEIGHT, each 2..4096");
			}
			else throw std::runtime_error("unknown option: " + key);
		}
		if (mode == deActive) mode = environmentPath.empty() ? Toon : PBR;
		if (environmentControl && environmentPath.empty()) throw std::runtime_error("environment controls require --env");
		if (!environmentPath.empty() && mode != PBR) throw std::runtime_error("--env requires --mode pbr");
		if (ao != Renderer::AoMode::Off && mode != Phong && mode != BlinnPhongWithShadow)
			throw std::runtime_error("--ao requires --mode phong or shadow");
		if (!aoOutput.empty() && ao == Renderer::AoMode::Off) throw std::runtime_error("--ao-map requires --ao ssao or brute");
		if (!explicitPost && mode == Toon) post = ToonEdge;

		auto scene = std::make_shared<Scene>();
		if (!environmentPath.empty())
		{
			auto environment = std::make_shared<EnvironmentLight>(environmentPath.c_str());
			environment->SetRotation(environmentRotation);
			environment->intensity = environmentStrength;
			scene->SetEnvironment(environment);
		}
		glm::vec3 eye, target;
		if (!models.empty())
		{
			if (!sceneName.empty()) throw std::runtime_error("choose --scene or --model");
			for (const auto& path : models) scene->LoadModel(path.c_str());
			eye = { 0.55f, 0.15f, 2.9f }; target = { 0, 0, 0 };
		}
		else if (sceneName.empty() || sceneName == "diablo")
		{
			scene->LoadModel("resource/models/diablo/d3.obj");
			scene->LoadModel("resource/models/floor_big/floor_big.obj");
			eye = { 1.0f, 1.15f, 3.7f }; target = { 0, -0.12f, 0 };
		}
		else if (sceneName == "head")
		{
			scene->LoadModel("resource/models/head/head/african_head.obj");
			scene->LoadModel("resource/models/head/eye_inner/african_head_eye_inner.obj");
			eye = { 0.55f, 0.15f, 2.9f }; target = { 0, 0, 0 };
		}
		else if (sceneName == "helmet")
		{
			scene->LoadModel("resource/models/FlightHelmet/FlightHelmet.obj");
			eye = { 0.66f, 0.76f, 0.84f }; target = { 0, 0.29f, 0 };
		}
		else throw std::runtime_error("unknown scene: " + sceneName);
		scene->InitLight({-1,-1,-1}, {1,1,1});
		scene->InitCamera(eye, target, {0,1,0}, glm::radians(45.0f), float(width)/height, 0.1f, 16.0f);

		Renderer renderer;
		renderer.InitShaders();
		renderer.SetViewport(0, 0, width, height);
		renderer.BeginScene(scene);
		renderer.BindShader(mode, ShaderStageType::defaultRaster);
		renderer.BindShader(post, ShaderStageType::postProcess);
		renderer.SetAoMode(ao);
		renderer.SetScanline(scanline);
		renderer.SetMsaa(msaa);
		renderer.SetHdr(hdr);
		renderer.SetAlpha(alphaMode, alphaCutoff);
		renderer.SetNormalSource(normal);
		renderer.SetTextureFilter(filter);
		renderer.SetShadowSize(shadowSize);
		renderer.SetShadowFilter(shadowFilter);
		renderer.Draw();
		if (!aoOutput.empty() && !renderer.SaveAo(aoOutput.c_str())) return 1;
		if (!output.empty()) return renderer.Image().write_tga_file(output.c_str()) ? 0 : 1;
		if (!renderer.SavePasses()) return 1;
		Window window("SoftRenderer", width, height);
		if (!window.Show(renderer.Image())) throw std::runtime_error(SDL_GetError());
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "SoftRenderer: " << error.what() << '\n';
		return 1;
	}
}
