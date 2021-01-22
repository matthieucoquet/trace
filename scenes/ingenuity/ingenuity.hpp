#pragma once
#include "engine/app.hpp"

namespace ingenuity
{

using App = Vr_app;
class Ingenuity : public App
{
public:
    Ingenuity();
    Ingenuity(const Ingenuity& other) = delete;
    Ingenuity(Ingenuity&& other) = delete;
    Ingenuity& operator=(const Ingenuity& other) = delete;
    Ingenuity& operator=(Ingenuity&& other) = delete;
    ~Ingenuity() = default;
};

}
