#pragma once

namespace sdf_editor
{

struct Scene;

class Gltf_loader
{
public:
    Gltf_loader(Scene& scene);
    Gltf_loader(const Gltf_loader& other) = delete;
    Gltf_loader(Gltf_loader&& other) = delete;
    Gltf_loader& operator=(const Gltf_loader& other) = delete;
    Gltf_loader& operator=(Gltf_loader&& other) = delete;
    ~Gltf_loader() = default;
private:

};

}