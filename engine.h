#pragma once
#include "common.h"

#include "window.h"
#include "context.h"
#include "renderer.h"
#include "scene.h"

class Engine
{
public:
    Engine();
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) = delete;
    ~Engine();

    void run();
    //void reset_renderer();
private:
    Window m_window;
    Context m_context;
    Scene m_scene;
    Renderer m_renderer;
};
