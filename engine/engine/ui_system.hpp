#pragma once
#include "core/system.hpp"
#include "core/scene.hpp"

#include <TextEditor.h>

#include <limits>

namespace sdf_editor
{

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
        entity,
        material,
        light
    };

    TextEditor m_editor;
    bool m_editor_init = false;

    Selected m_selected{ Selected::engine_shader };
    int m_selected_id{ 0 };
    size_t m_selected_scene_group = Entity::empty_id;

    int entity_node(Entity& entity, int id = 0);
    void shader_text(Shader_file& shader_file);

    void record_selected(Scene& scene);
};

}