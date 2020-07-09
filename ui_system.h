#pragma once
#include "system.h"

class Ui_system : public System
{
public:
    Ui_system() = default;
    Ui_system(const Ui_system& other) = delete;
    Ui_system(Ui_system&& other) = delete;
    Ui_system& operator=(const Ui_system& other) = delete;
    Ui_system& operator=(Ui_system&& other) = delete;
    ~Ui_system() override final {}
    void step(Scene& scene) override final;
};