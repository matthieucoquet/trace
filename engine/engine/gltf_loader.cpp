#include "gltf_loader.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <string>
#include <span>
#include "core/scene.hpp"

#pragma warning(disable : 4702)
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_JSON
#include <nlohmann/json.hpp>
#include <tiny_gltf.h>

constexpr bool verbose = true;

static glm::vec3 to_vec3(std::vector<double> v) {
    return glm::vec3(float(v[0]), float(v[1]), float(v[2]));
}

static glm::quat to_quat(std::vector<double> q) {
    return glm::quat(float(q[3]), float(q[0]), float(q[1]), float(q[2]));
}

struct Gltf_character
{
    const tinygltf::Model& model;
    std::vector<int> parents{};
    std::vector<Transform> transforms{};
    std::vector<int> joints{};

    Gltf_character(const tinygltf::Model& m) :
        model(m),
        joints(m.nodes.size(), 0)
    {
        int root_id = model.scenes[0].nodes[0];
        const tinygltf::Node& root = model.nodes[root_id];
        transforms.reserve(m.nodes.size());
        parents.reserve(m.nodes.size());
        parents.push_back(0);
        joints[root_id] = 0;
        create(root);

    }

    int create(const tinygltf::Node& node, int current_id = 0)
    {
        int child_id = current_id + 1;
        for (int gltf_child_id : node.children)
        {
            parents.push_back(current_id);
            transforms.push_back(Transform{
                .position = to_vec3(node.translation),
                .rotation = to_quat(node.rotation)
            });
            joints[gltf_child_id] = child_id;
            child_id = create(model.nodes[gltf_child_id], child_id);
        }
        return child_id;
    }
};

Gltf_loader::Gltf_loader(Scene& /*scene*/)
{
    const std::filesystem::path data_directory(DATA_SOURCE);
    const std::filesystem::path file = data_directory / "0007_Walking001.glb";

    tinygltf::Model model{};
    std::string err;
    std::string warn;

    tinygltf::TinyGLTF loader{};
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, file.string());

    if (!warn.empty()) {
        fmt::print("Warn: {}\n", warn);
    }
    if (!err.empty()) {
        fmt::print("Err: {}\n", err);
    }
    if (!ret) {
        fmt::print("Failed to parse glTF\n");
        return;
    }
    if constexpr (verbose) {
        fmt::print("Loaded file: {}\n", file.string());
        fmt::print("Animations size: {}\n", model.animations.size());
    }

    //Gltf_character character(model);
    //scene.characters.emplace_back(Character{ .transforms = std::move(character.transforms) });

    /* for (const tinygltf::Animation& animation : model.animations)
    {
        fmt::print("samplers: {}\n", animation.samplers.size());
        fmt::print("channels: {}\n", animation.channels.size());
        for (const tinygltf::AnimationChannel& channel : animation.channels)
        {
            //const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
            //std::span<float>

        }
    }*/
}
