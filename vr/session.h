#pragma once

#include "common.h"

namespace vulkan {
class Context;
}

namespace vr
{

class Context;

class Session
{
public:
    xr::Session session;

    Session(Context& vr_context, vulkan::Context& vulkan_context);
    ~Session();
private:
};

}