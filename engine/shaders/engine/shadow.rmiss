#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 1) rayPayloadInEXT float shadow_payload;

void main()
{
    shadow_payload = 1.0;
}
