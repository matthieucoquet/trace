#pragma once
#include "core/system.h"

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
    Shader* m_selected_shader = nullptr;

    void add_leaf(const char* label, Shader* shader);
    void shader_text(Shader_file& shader_file);
};