#pragma once
#include "core/system.hpp"
#include <limits>

struct Shader;
struct Shader_file;

class Ui_system : public System
{
public:
    Ui_system();
    Ui_system(const Ui_system& other) = delete;
    Ui_system(Ui_system&& other) = delete;
    Ui_system& operator=(const Ui_system& other) = delete;
    Ui_system& operator=(Ui_system&& other) = delete;
    ~Ui_system() override final = default;
    void step(Scene& scene) override final;
private:
    Shader* m_selected_shader{ nullptr };
    size_t m_selected_object{ std::numeric_limits<size_t>::max() };

    void add_leaf(const char* label, Shader* shader);
    void shader_text(Shader_file& shader_file);

    void record_selected(Scene& scene);
};