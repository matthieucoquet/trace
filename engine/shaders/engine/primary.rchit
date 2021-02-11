#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "map_function"
#include "raymarch.glsl"
#include "lighting.glsl"

layout(location = 0) rayPayloadInEXT vec3 hit_value;

layout(binding = 2, set = 0, scalar) buffer Materials { Material m[]; } materials;

void main()
{
    vec3 global_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
#ifdef DEBUG_SDF
    Hit hit = map(vec3(gl_WorldToObjectEXT * vec4(global_position, 1.0f)));

    vec3 color = hit.dist > 0 ? vec3(0.4, 0.8, 0.4) : vec3(0.4, 0.4, 0.8);
    color = (0.5 + 0.5 * cos (10.0 * 6.283 * hit.dist)) * color;

    hit_value = color;
#else
    float scale = 1 / length(gl_WorldToObjectEXT * vec4(1.0, 0.0, 0.0, 0.0));
    vec3 local_position = vec3(gl_WorldToObjectEXT * vec4(global_position, 1.0f));
    vec3 normal = normal(local_position);
    vec3 color = lighting(global_position, local_position, vec3(scene_global.transform * vec4(global_position, 1.0)), normal, gl_WorldToObjectEXT, scale);
    
    if (gl_HitKindEXT < UNKNOW) {
        Material material = materials.m[nonuniformEXT(gl_HitKindEXT)];
        hit_value = color * material.color;
    }
    else {
        hit_value = color * get_color(local_position);
    }
#endif
}



