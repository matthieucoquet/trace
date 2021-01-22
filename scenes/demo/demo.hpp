#pragma once
#include "engine/app.hpp"

namespace demo
{

using App = sdf_editor::Vr_app;

class Demo : public App
{
public:
    Demo();
    Demo(const Demo& other) = delete;
    Demo(Demo&& other) = delete;
    Demo& operator=(const Demo& other) = delete;
    Demo& operator=(Demo&& other) = delete;
    ~Demo() = default;
};

}