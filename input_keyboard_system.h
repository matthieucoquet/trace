#pragma once
#include "core/system.h"

struct GLFWwindow;

class Input_keyboard_system : public System
{
public:
    GLFWwindow* window;
    Input_keyboard_system(GLFWwindow* window);
    Input_keyboard_system(const Input_keyboard_system& other) = delete;
    Input_keyboard_system(Input_keyboard_system&& other) = delete;
    Input_keyboard_system& operator=(const Input_keyboard_system& other) = delete;
    Input_keyboard_system& operator=(Input_keyboard_system&& other) = delete;
    ~Input_keyboard_system() override final = default;
    void step(Scene& scene) override final;
private:
    double m_time = 0.0;
};
