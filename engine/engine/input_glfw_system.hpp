#pragma once
#include "core/system.hpp"

struct GLFWwindow;

class Input_glfw_system : public System
{
public:
    GLFWwindow* window;
    Input_glfw_system(GLFWwindow* window);
    Input_glfw_system(const Input_glfw_system& other) = delete;
    Input_glfw_system(Input_glfw_system&& other) = delete;
    Input_glfw_system& operator=(const Input_glfw_system& other) = delete;
    Input_glfw_system& operator=(Input_glfw_system&& other) = delete;
    ~Input_glfw_system() override final;
    void step(Scene& scene) override final;
private:
    double m_time = 0.0;

    void update_mouse_pos_and_buttons(Scene& scene);
};
