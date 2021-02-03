#pragma once
#include "engine/app.hpp"

namespace tournesol
{

using App = sdf_editor::Vr_app;

class Tournesol : public App
{
public:
    Tournesol();
    Tournesol(const Tournesol& other) = delete;
    Tournesol(Tournesol&& other) = delete;
    Tournesol& operator=(const Tournesol& other) = delete;
    Tournesol& operator=(Tournesol&& other) = delete;
    ~Tournesol() = default;
};

}