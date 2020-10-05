#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "miss.glsl"
#include "raymarch.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec4 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;

layout(binding = 3, set = 0, scalar) buffer Materials { Material m[]; } materials;

void main()
{
    Ray ray = Ray(gl_WorldRayOriginEXT, gl_WorldRayDirectionEXT);

    Hit hit = raymarch(ray);
    if (hit.dist > 0.0)
    {
        vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * hit.dist;
        vec3 light_pos = vec3(10.0, 30.0, 4.0);
        vec3 light_dir = normalize(light_pos - position);
        vec3 light_color = vec3(1.0, 1.0, 1.0);

        vec3 normal = normal(position);
        vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
        vec3 ambient = 0.2 * light_color;

        vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
        shadow_payload = 0.0;
        if (dot(normal, light_dir) > 0)
        {
            shadow_payload = 1.0;
            traceRayEXT(topLevelAS,  // acceleration structure
                        gl_RayFlagsSkipClosestHitShaderEXT,
                        0xF0,        // cullMask
                        1,           // sbtRecordOffset
                        0,           // sbtRecordStride
                        1,           // missIndex
                        position,    // ray origin
                        0.1,         // ray min range
                        light_dir,   // ray direction
                        100.0,       // ray max range
                        1            // payload (location = 1)
                        );
        }
        
        float front = dot(position - scene_global.ui_position, scene_global.ui_normal) <= 0.0f ? 0.0f : 1.0f;
        vec3 color = (0.7 + 0.7 * shadow_payload) * (ambient + diffuse);
        vec3 spec = vec3(max(dot(normal, reflection), 0.0));
        color = color + shadow_payload * spec;

        Material material = materials.m[nonuniformEXT(hit.material_id)];
        hit_value = vec4(color * material.color, front);
    }
    else
    {
        hit_value = vec4(0.0, 0.0, 0.2, 0.0);
    }
}