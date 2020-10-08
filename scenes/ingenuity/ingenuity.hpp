#pragma once
#include "engine/engine.hpp"

namespace ingenuity
{

class Ingenuity : public Engine
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