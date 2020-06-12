#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_primitives.glsl"
#include "sphere.glsl"
#include "common_raymarch.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec3 attributes;

layout(binding = 2, set = 0, scalar) buffer Primitives { Primitive p[]; } primitives;

void main()
{
    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 light_pos = vec3(10.0, 30.0, 4.0);
    vec3 light_dir = normalize(light_pos - position);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
  
    Primitive primitive = primitives.p[gl_InstanceID];
    vec3 normal = normal(position - primitive.center);
    vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
    vec3 ambient = 0.2 * light_color;

    vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
    if (hit_value.x == 0.0)
    {
        hit_value = vec3(0.01, 0.01, 0.01);
        traceRayEXT(topLevelAS,  // acceleration structure
                    gl_RayFlagsOpaqueEXT,       // rayFlags
                    0xFF,        // cullMask
                    0,           // sbtRecordOffset
                    0,           // sbtRecordStride
                    0,           // missIndex
                    position,    // ray origin
                    0.1,         // ray min range
                    reflection,  // ray direction
                    10.0,        // ray max range
                    0            // payload (location = 1)
                    );
        }

    vec3 spec = hit_value * max(dot(normal, reflection), 0.0);
    hit_value = (spec + ambient + diffuse) * vec3(0.5, 0.5, 0.1);
}