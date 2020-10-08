#pragma once
#include "engine/engine.hpp"

namespace demo
{

class Demo : public Engine
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