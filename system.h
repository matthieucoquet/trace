#pragma once

class Scene;

class System
{
public:
    virtual ~System() = default;
    virtual void step(Scene& scene) = 0;
};