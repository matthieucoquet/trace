#pragma once
#include "core/system.hpp"
#include <limits>

namespace sdf_editor
{

struct Shader;
struct Shader_file;

class Ui_system final : public System
{
public:
    Ui_system();
    Ui_system(const Ui_system& other) = delete;
    Ui_system(Ui_system&& other) = delete;
    Ui_system& operator=(const Ui_system& other) = delete;
    Ui_system& operator=(Ui_system&& other) = delete;
    ~Ui_system() override = default;
    void step(Scene& scene) override final;
private:
    enum class Selected
    {
        engine_shader,
        scenes_shader,
        object,
        material,
        light
    };

    Selected m_selected{ Selected::engine_shader };
    int m_selected_id{ 0 };

    void shader_text(Shader_file& shader_file);

    void record_selected(Scene& scene);
};

}