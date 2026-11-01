#include "Model2.h"
#include <iostream>

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

    size_t slash = path.find_last_of("/");
    if (slash  == std::string::npos)
    {
        return;
    }
    ParseMaterial(mats, path.substr(0, slash) + reader_config.mtl_search_path);
    
    for (size_t s = 0; s < shapes.size(); s++) {
        uint32_t nface = (uint32_t)shapes[s].mesh.num_face_vertices.size();
        Primative primative { nface };
        // fall back to an empty material when the obj has no mtl / no material id, avoids out-of-range access
        int matId = shapes[s].mesh.material_ids.empty() ? -1 : shapes[s].mesh.material_ids[0];
        if (matId >= 0 && matId < (int)materials.size()) {
            primative.SetMaterial(materials[matId]);
        }
        else {
            primative.SetMaterial(std::make_shared<Material>());
        }
        size_t index_offset = 0;
        for (size_t f = 0; f < nface; f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            
            for (size_t v = 0; v < fv; v++) {
              
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                primative.InerstPosition(f * 3 + v, vx, vy, vz);                
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    primative.InsertNormal(f * 3 + v, nx, ny, nz);
                }
      
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    primative.InsertUV(f * 3 + v, tx, 1.0f - ty);
                }
            }
            index_offset += fv;
        }
        primatives.push_back(primative);
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

        if (!mat.bump_texname.empty())
        {
            material->normal = std::make_shared<Texture>((path + "/" + mat.bump_texname).c_str());
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