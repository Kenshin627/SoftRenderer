#include "Model2.h"
#include <iostream>
#include <fstream>

Model2::Model2(const std::string& path)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = ""; 
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(path, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& mats = reader.GetMaterials();

    size_t slash = path.find_last_of("/\\");
    ParseMaterial(mats, slash == std::string::npos ? "." : path.substr(0, slash));

    // tutorial assets (head/diablo/floor) ship no .mtl: fall back to the tinyrenderer
    // suffix convention (<model>_diffuse/_nm_tangent/_spec.tga) so they render textured
    std::shared_ptr<Material> fallbackMaterial = std::make_shared<Material>();
    if (materials.empty()) {
        size_t dot = path.find_last_of(".");
        if (dot != std::string::npos) {
            std::string base = path.substr(0, dot);
            auto tryLoad = [&fallbackMaterial](const std::string& file, std::shared_ptr<Texture> Material::* slot) {
                std::ifstream f(file);
                if (f.good()) fallbackMaterial.get()->*slot = std::make_shared<Texture>(file.c_str());
            };
            tryLoad(base + "_diffuse.tga", &Material::diffuse);
            if (!fallbackMaterial->diffuse) tryLoad(base + "_diff.tga", &Material::diffuse);
            tryLoad(base + "_nm.tga", &Material::objectNormal);
            tryLoad(base + "_nm_tangent.tga", &Material::normal);
            tryLoad(base + "_spec.tga", &Material::specular);
            tryLoad(base + "_glow.tga", &Material::emissive);
        }
    }

    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh; // ObjReader triangulates polygons.
        const size_t count = mesh.num_face_vertices.size();
        size_t first = 0;
        while (first < count) {
            int id = mesh.material_ids[first];
            size_t end = first + 1;
            while (end < count && mesh.material_ids[end] == id) ++end;
            primatives.emplace_back(end - first);
            auto& primitive = primatives.back();
            primitive.SetMaterial(id >= 0 && size_t(id) < materials.size() ? materials[id] : fallbackMaterial);
            for (size_t face = first; face < end; ++face) {
                glm::vec3 points[3];
                for (int k = 0; k < 3; ++k) {
                    const auto& index = mesh.indices[3 * face + k];
                    const auto* p = &attrib.vertices.at(3 * size_t(index.vertex_index));
                    points[k] = {p[0], p[1], p[2]};
                }
                glm::vec3 faceNormal = glm::cross(points[1] - points[0], points[2] - points[0]);
                for (int k = 0; k < 3; ++k) {
                    const auto& index = mesh.indices[3 * face + k];
                    size_t corner = 3 * (face - first) + k;
                    primitive.InerstPosition(corner, points[k].x, points[k].y, points[k].z);
                    glm::vec3 normal = faceNormal;
                    if (index.normal_index >= 0) {
                        const auto* n = &attrib.normals.at(3 * size_t(index.normal_index));
                        normal = {n[0], n[1], n[2]};
                    }
                    primitive.InsertNormal(corner, normal.x, normal.y, normal.z);
                    if (index.texcoord_index >= 0) {
                        const auto* uv = &attrib.texcoords.at(2 * size_t(index.texcoord_index));
                        primitive.InsertUV(corner, uv[0], 1.0f - uv[1]);
                    }
                }
            }
            first = end;
        }
    }
}

void Model2::PushPrimative(const Primative& p)
{
	primatives.push_back(p);
}

void Model2::ParseMaterial(const std::vector<tinyobj::material_t>& mats, const std::string& path)
{
    for (const auto& mat : mats)
    {
        std::shared_ptr<Material> material = std::make_shared<Material>();
        if (!mat.ambient_texname.empty())
        {
            material->ao = std::make_shared<Texture>((path + "/" + mat.ambient_texname).c_str());
        }
        if (!mat.diffuse_texname.empty())
        {
            material->diffuse = std::make_shared<Texture>((path + "/" + mat.diffuse_texname).c_str());
        }

        if (!mat.specular_texname.empty())
        {
            material->specular = std::make_shared<Texture>((path + "/" + mat.specular_texname).c_str());
        }

        if (!mat.emissive_texname.empty())
        {
            material->emissive = std::make_shared<Texture>((path + "/" + mat.emissive_texname).c_str());
        }
        const auto& normalMap = mat.normal_texname.empty() ? mat.bump_texname : mat.normal_texname;
        if (!normalMap.empty())
        {
            material->normal = std::make_shared<Texture>((path + "/" + normalMap).c_str());
        }

        if (!mat.roughness_texname.empty())
        {
            material->roughness = std::make_shared<Texture>((path + "/" + mat.roughness_texname).c_str());
        }

        if (!mat.metallic_texname.empty())
        {
            material->metalness = std::make_shared<Texture>((path + "/" + mat.metallic_texname).c_str());
        }
        materials.push_back(material);
    }       
}