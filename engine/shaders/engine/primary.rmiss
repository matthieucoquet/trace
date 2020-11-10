#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "miss.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec4 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;

layout(binding = 3, set = 0, scalar) buffer Materials { Material m[]; } materials;
layout(binding = 4, set = 0, scalar) buffer Lights { Light l[]; } lights;

Hit raymarch_miss(in Ray ray)
{
    float t = 0.0f;
    for (int i = 0; i < 512 && t < 500.0; i++)
    {
        Hit hit = map_miss(ray.origin + t * ray.direction);
        if(hit.dist < 0.01) {
            return Hit(t, hit.material_id);
        }
        t += ADVANCE_RATIO_MISS * hit.dist;
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
    Ray ray = Ray(gl_WorldRayOriginEXT, gl_WorldRayDirectionEXT);

    Hit hit = raymarch_miss(ray);
    if (hit.dist > 0.0)
    {
        vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * hit.dist;

        vec3 normal = normal(position);
        vec3 color  = vec3(0.0);        
        for (int i = 0; i < scene_global.nb_lights; i++)
        {
            Light light = lights.l[nonuniformEXT(i)];  // nonuniformEXT shouldnt be needed
            vec3 light_dir = normalize(light.position - position);
            vec3 diffuse = max(dot(normal, light_dir), 0.0) * light.color;
            vec3 ambient = 0.1 * light.color;

            vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
            shadow_payload = 0.0;
            //if (dot(normal, light_dir) > 0)
            /*if (true)
            {
                shadow_payload = 1.0;
                traceRayEXT(topLevelAS,  // acceleration structure
                            gl_RayFlagsSkipClosestHitShaderEXT,
                            0xFF,        // cullMask
                            1,           // sbtRecordOffset
                            0,           // sbtRecordStride
                            1,           // missIndex
                            position,    // ray origin
                            0.5,         // ray min range
                            light_dir,   // ray direction
                            10.0,       // ray max range
                            1            // payload (location = 1)
                            );
            }*/
            /*if (hit_value.x == 0.0)
            {
                hit_value.x = 0.1;
                vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
                traceRayEXT(topLevelAS,  // acceleration structure
                    gl_RayFlagsOpaqueEXT,
                    0xFF,        // cullMask
                    0,           // sbtRecordOffset
                    0,           // sbtRecordStride
                    0,           // missIndex
                    position,    // ray origin
                    0.001,       // ray min range
                    reflection,   // ray direction
                    100.0,       // ray max range
                    0            // payload (location = 1)
                    );
            }*/
            vec3 spec = vec3(max(dot(normal, reflection), 0.0));
            color = ambient + shadow_payload * (spec + diffuse);
        }
        
        Material material = materials.m[nonuniformEXT(hit.material_id)];
        float front = dot(position - scene_global.ui_position, scene_global.ui_normal) <= 0.0f ? 0.0f : 1.0f;
        //hit_value = vec4(color * material.color, front);
        hit_value = vec4(color, front);
    }
    else
    {
        hit_value = vec4(background_miss(ray.origin + ray.direction), 0.0);
    }
}