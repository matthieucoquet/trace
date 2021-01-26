#pragma once
#include "vk_common.hpp"
#include "vma_image.hpp"
#include <string_view>

namespace sdf_editor::vulkan
{
class Context;

class Texture
{
public:
    int height;
    int width;
    Vma_image image;
    vk::ImageView image_view;

    Texture(Context& context, std::string_view filename);
    Texture(Context& context, vk::Extent2D extent);
    Texture(const Texture& other) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(const Texture& other) = delete;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

private:
    vk::Device m_device;
};

class Sampler
{
public:
    vk::Sampler sampler;

    Sampler(Context& context);
    Sampler(const Texture& other) = delete;
    Sampler(Texture&& other) = delete;
    Sampler& operator=(const Texture& other) = delete;
    Sampler& operator=(Texture&& other) = delete;
    ~Sampler();

private:
    vk::Device m_device;
};

}
