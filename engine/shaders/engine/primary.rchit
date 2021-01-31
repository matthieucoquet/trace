#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "map_function"
#include "raymarch.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec3 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;

layout(binding = 2, set = 0, scalar) buffer Materials { Material m[]; } materials;
layout(binding = 3, set = 0, scalar) buffer Lights { Light l[]; } lights;

void main()
{
    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
#ifdef DEBUG_SDF
    Hit hit = map(vec3(gl_WorldToObjectEXT * vec4(position, 1.0f)));

    vec3 color = hit.dist > 0 ? vec3(0.4, 0.8, 0.4) : vec3(0.4, 0.4, 0.8);
    color = (0.5 + 0.5 * cos (10.0 * 6.283 * hit.dist)) * color;

    hit_value = vec4(color, front);
#else
    vec3 color  = vec3(0.0);
    
    vec3 model_position = vec3(gl_WorldToObjectEXT * vec4(position, 1.0f));
    vec3 normal = normal(model_position);
    vec3 view_dir = normalize(gl_WorldToObjectEXT * vec4(gl_WorldRayOriginEXT, 1.0f) - model_position);

    for (int i = 0; i < scene_global.nb_lights; i++)
    {
        Light light = lights.l[nonuniformEXT(i)];  // nonuniformEXT shouldnt be needed
        vec3 light_dir = normalize(gl_WorldToObjectEXT * vec4(light.position, 1.0f) - model_position);

        vec3 diffuse = max(dot(normal, light_dir), 0.0) * light.color;
        vec3 ambient = 0.1 * light.color;

        vec3 halfway = normalize(light_dir + view_dir);
        vec3 spec = pow(max(dot(normal, halfway), 0.0), 32.0) * light.color;

        shadow_payload = 0.0;
        if (dot(normal, light_dir) > 0)
        {
            light_dir =  gl_ObjectToWorldEXT * vec4(light_dir, 0.0f);
            shadow_payload = 1.0;
            traceRayEXT(topLevelAS,  // acceleration structure
                        gl_RayFlagsSkipClosestHitShaderEXT,
                        0xFF,        // cullMask
                        1,           // sbtRecordOffset
                        0,           // sbtRecordStride
                        1,           // missIndex
                        position,    // ray origin
                        0.001,       // ray min range
                        light_dir,   // ray direction
                        100.0,       // ray max range
                        1            // payload (location = 1)
                        );
        }
        
        //color = ambient + hit_value.xyz;
        color = color + ambient + shadow_payload * (spec + diffuse);
    }
    
    if (gl_HitKindEXT < UNKNOW) {
        Material material = materials.m[nonuniformEXT(gl_HitKindEXT)];
        hit_value = color * material.color;
    }
    else {
        hit_value = get_color(model_position);
    }
#endif
}
