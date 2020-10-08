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
    bool m_selected_engine{ true };
    int m_selected_shader{ 0 };
    int m_selected_object{ std::numeric_limits<int>::max() };

    void shader_text(Shader_file& shader_file);

    void record_selected(Scene& scene);
};