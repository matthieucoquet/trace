#pragma once
#include "vr_common.hpp"

class Scene;

namespace vr
{

class Suggested_binding
{
public:
    std::vector<xr::ActionSuggestedBinding> suggested_binding_oculus;

    Suggested_binding(xr::Instance instance) : m_instance(instance) 
    {
        suggested_binding_oculus.reserve(8);
    }

    ~Suggested_binding()
    {
        m_instance.suggestInteractionProfileBindings(xr::InteractionProfileSuggestedBinding{
            .interactionProfile = m_instance.stringToPath("/interaction_profiles/oculus/touch_controller"),
            .countSuggestedBindings = static_cast<uint32_t>(suggested_binding_oculus.size()),
            .suggestedBindings = suggested_binding_oculus.data() });
    }

private:
    xr::Instance m_instance;
};

class Input_system
{
public:
    enum class Interaction_profile {
        oculus
    };


    Input_system() = default;
    Input_system(const Input_system& other) = delete;
    Input_system(Input_system&& other) = delete;
    Input_system& operator=(const Input_system& other) = delete;
    Input_system& operator=(Input_system&& other) = delete;
    virtual ~Input_system() = default;

    virtual void suggest_interaction_profile(xr::Instance instance, Suggested_binding& suggested_bindings) = 0;

    virtual void step(Scene& scene, xr::Session session, xr::Time display_time, xr::Space base_space, float offset_space_y) = 0;
};

}