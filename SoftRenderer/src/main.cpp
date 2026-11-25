#include "window/Window.h"
#include <iostream>
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

void usage()
{
	std::cout << "SoftRenderer [--scene diablo|head|helmet | --model file.obj ...] [--size WIDTHxHEIGHT] [--output frame.tga]\n"
		"  --mode albedo|emission|lit|wire|barycentric|flat|gouraud|smooth|phong|blinn|normal|shadow|toon|depth|pbr\n"
		"  --filter nearest|bilinear|mipmap|trilinear   (default: nearest)\n"
		"  --normals auto|vertex|object|tangent   --raster bbox|scanline\n"
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
		ShaderType mode = Toon, post = deActive;
		TextureFilter filter = TextureFilter::Nearest;
		NormalSource normal = NormalSource::Auto;
		auto ao = Renderer::AoMode::Off;
		bool explicitPost = false, scanline = false;
		int width = 1920, height = 1080;
		for (int i = 1; i < argc; ++i)
		{
			std::string key = argv[i];
			if (key == "--help") { usage(); return 0; }
			if (++i == argc) throw std::runtime_error("missing value for " + key);
			std::string value = argv[i];
			if (key == "--scene") sceneName = value;
			else if (key == "--model") models.push_back(value);
			else if (key == "--mode") mode = choice(modes, value);
			else if (key == "--post") { post = choice(effects, value); explicitPost = true; }
			else if (key == "--filter") filter = choice(filters, value);
			else if (key == "--normals") normal = choice(normals, value);
			else if (key == "--ao") ao = choice(occlusion, value);
			else if (key == "--ao-map") aoOutput = value;
			else if (key == "--output") output = value;
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
		if (ao != Renderer::AoMode::Off && mode != Phong && mode != BlinnPhongWithShadow)
			throw std::runtime_error("--ao requires --mode phong or shadow");
		if (!aoOutput.empty() && ao == Renderer::AoMode::Off) throw std::runtime_error("--ao-map requires --ao ssao or brute");
		if (!explicitPost && mode == Toon) post = ToonEdge;

		auto scene = std::make_shared<Scene>();
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
		renderer.SetNormalSource(normal);
		renderer.SetTextureFilter(filter);
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
