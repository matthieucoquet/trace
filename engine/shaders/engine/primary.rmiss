#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "common_types.glsl"
#include "miss.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec3 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;

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

float soft_shadow(in Ray ray, in float factor)
{
    float res = 1.0;
    float t = 0.05;
    for (int i = 0; i < 16; i++)
    {
        float distance = map_miss(ray.origin + t * ray.direction).dist;
        if(distance < 0.001) {
            return 0.0;
        }
        res = min(res, factor * distance / t);
        t += ADVANCE_RATIO_MISS * distance;
    }
    return res;
}

void main()
{
    Ray ray = Ray(vec3(scene_global.transform * vec4(gl_WorldRayOriginEXT, 1.0)),
                  vec3(scene_global.transform * vec4(gl_WorldRayDirectionEXT, 0.0)));

    Hit hit = raymarch_miss(ray);
    if (hit.dist > 0.0)
    {
        vec3 model_position = ray.origin + hit.dist * ray.direction;
        vec3 normal = normal(model_position);
        vec3 view_dir = normalize(vec3(scene_global.transform * vec4(gl_WorldRayOriginEXT, 1.0f)) - model_position);
        vec3 color = vec3(0.0);        
        for (int i = 0; i < scene_global.nb_lights; i++)
        {
            Light light = lights.l[nonuniformEXT(i)];  // nonuniformEXT shouldnt be needed
            vec3 light_dir = normalize(vec3(scene_global.transform * vec4(light.position, 1.0f)) - model_position);
            //vec3 light_dir = normalize(light.position - position);
            vec3 diffuse = max(dot(normal, light_dir), 0.0) * light.color;
            vec3 ambient = 0.1 * light.color;

            //vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);

            vec3 halfway = normalize(light_dir + view_dir);
            vec3 spec = pow(max(dot(normal, halfway), 0.0), 32.0) * light.color;

            shadow_payload = 0.0;
            if (dot(normal, light_dir) > 0)
            {
                shadow_payload = soft_shadow(Ray(model_position, light_dir), 128.0);
                // TODO put inverse in uniform
                vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * hit.dist;
                light_dir = vec3(inverse(scene_global.transform) * vec4(light_dir, 0.0f));
                //shadow_payload = 1.0;
                traceRayEXT(topLevelAS,  // acceleration structure
                            gl_RayFlagsSkipClosestHitShaderEXT,
                            0xFF,        // cullMask
                            1,           // sbtRecordOffset
                            0,           // sbtRecordStride
                            1,           // missIndex
                            position,    // ray origin
                            0.05,         // ray min range
                            light_dir,   // ray direction
                            100.0,        // ray max range
                            1            // payload (location = 1)
                            );
            }
            //vec3 spec = vec3(max(dot(normal, reflection), 0.0));
            color = color + ambient + shadow_payload * (spec + diffuse);
        }
        
        if (hit.material_id < UNKNOW) {
            Material material = materials.m[nonuniformEXT(hit.material_id)];
            hit_value = vec3(color * material.color);
        }
        else {
            hit_value = color * get_color_miss(model_position);
        }

    }
    else
    {
        hit_value = background_miss(ray.direction);
    }
}
