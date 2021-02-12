#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common_types.glsl"
#include "lighting.glsl"

layout(location = 0) rayPayloadInEXT vec3 hit_value;

layout(binding = 2, set = 0, scalar) buffer Materials { Material m[]; } materials;

Hit raymarch_miss(in Ray ray)
{
    float len = length(ray.direction);
    float t = 0.0f;
    for (int i = 0; i < 512 && t < 500.0; i++)
    {
        Hit hit = map_miss(ray.origin + t * ray.direction);
        if(hit.dist < 0.01) {
            return Hit(t, hit.material_id);
        }
        t += ADVANCE_RATIO_MISS * hit.dist / len;
    }
    return Hit(-1.0, 0);
}

vec3 normal(in vec3 position)
{
    vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = 0.0025;
    return normalize(
        e.xyy * map_miss(position + e.xyy * eps).dist +
        e.yyx * map_miss(position + e.yyx * eps).dist +
        e.yxy * map_miss(position + e.yxy * eps).dist +
        e.xxx * map_miss(position + e.xxx * eps).dist);
}

void main()
{
    Ray ray = Ray(vec3(scene_global.transform * vec4(gl_WorldRayOriginEXT, 1.0)),
                  vec3(scene_global.transform * vec4(gl_WorldRayDirectionEXT, 0.0)));
    float scale = length(ray.direction);
    Hit hit = raymarch_miss(ray);
    if (hit.dist > 0.0)
    {
        vec3 local_position = ray.origin + hit.dist * ray.direction;
        vec3 local_normal = normal(local_position);            
        vec3 global_normal = normalize(vec3(inverse(scene_global.transform) * vec4(local_normal, 0.0f)));

        vec3 global_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * hit.dist;
        vec3 color = lighting(global_position, local_position, local_position, 
            global_normal, local_normal, mat4x3(scene_global.transform), scale);
                
        if (hit.material_id < UNKNOW) {
            Material material = materials.m[nonuniformEXT(hit.material_id)];
            hit_value = vec3(color * material.color);
        }
        else {
            hit_value = color * get_color_miss(local_position);
        }

    }
    else
    {
        hit_value = background_miss(ray.direction);
    }
}
