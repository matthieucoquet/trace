#pragma once
#include <imgui.h>

struct GLFWwindow;

namespace vr
{

class Imgui_input
{
public:
    GLFWwindow* window;
    Imgui_input(GLFWwindow* window);
    void step();
private:
    double m_time = 0.0;
};

}
