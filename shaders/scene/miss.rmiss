#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_primitives.glsl"
#include "miss.glsl"
#include "common_raymarch.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec4 hit_value;

void main()
{
    Ray ray = Ray(gl_WorldRayOriginEXT, gl_WorldRayDirectionEXT);

    float t = raymarch(ray);
    if (t > 0.0)
    {
        vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * t;
        vec3 light_pos = vec3(10.0, 30.0, 4.0);
        vec3 light_dir = normalize(light_pos - position);
        vec3 light_color = vec3(1.0, 1.0, 1.0);

        vec3 normal = normal(position);
        vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
        vec3 ambient = 0.2 * light_color;

        vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
        /*if (hit_value.x == 0.0)
        {
            hit_value = vec3(0.01, 0.01, 0.01);
            traceRayEXT(topLevelAS,  // acceleration structure
                        gl_RayFlagsOpaqueEXT,       // rayFlags
                        0xFF,        // cullMask
                        0,           // sbtRecordOffset
                        0,           // sbtRecordStride
                        0,           // missIndex
                        position,    // ray origin
                        0.5,         // ray min range
                        reflection,  // ray direction
                        100.0,        // ray max range
                        0            // payload (location = 1)
                        );
            }*/

        vec3 spec = 1.12 * hit_value.xyz * max(dot(normal, reflection), 0.0);
        float front = dot(position - scene_global.ui_position, scene_global.ui_normal) <= 0.0f ? 0.0f : 1.0f;
        hit_value = vec4((spec + ambient + diffuse) * vec3(0.4, 0.4, 0.4), front);
    }
    else
    {
        hit_value = vec4(0.0, 0.0, 0.2, 0.0);
    }
}