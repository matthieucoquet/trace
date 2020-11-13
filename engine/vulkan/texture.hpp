#pragma once
#include "vk_common.hpp"
#include "vma_image.hpp"
#include <string_view>

namespace vulkan
{
class Context;

class Texture
{
public:
    int height;
    int width;
    Vma_image image;
    vk::ImageView image_view;
    vk::Sampler sampler;

    Texture(Context& context, std::string_view filename);
    Texture(const Texture& other) = delete;
    Texture(Texture&& other) = delete;
    Texture& operator=(const Texture& other) = delete;
    Texture& operator=(Texture&& other) = delete;
    ~Texture();

private:
    vk::Device m_device;
};

}
