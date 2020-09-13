#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "map_function"
#include "raymarch.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec4 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;

layout(binding = 2, set = 0, scalar) buffer Objects { Object o[]; } objects;

void main()
{
    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    float front = dot(position - scene_global.ui_position, scene_global.ui_normal) <= 0.0f ? 0.0f : 1.0f;
    vec3 light_pos = vec3(10.0, 30.0, 4.0);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
  
    Object object = objects.o[gl_InstanceID];
    vec3 model_position = vec3(object.world_to_model * vec4(position, 1.0f));
    vec3 normal = normal(model_position);
    vec3 light_dir = normalize(vec3(object.world_to_model * vec4(light_pos, 1.0f)) - model_position);
    vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
    vec3 ambient = 0.1 * light_color;

    //vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
    vec3 view_dir = normalize(vec3(object.world_to_model * vec4(gl_WorldRayOriginEXT, 1.0f)) - model_position);
    vec3 halfway = normalize(light_dir + view_dir);  
    vec3 spec = pow(max(dot(normal, halfway), 0.0), 32.0) * light_color;
    shadow_payload = 0.0;
    if (dot(normal, light_dir) > 0)
    {
        light_dir = vec3(vec4(light_dir, 0.0f) * object.world_to_model);
        shadow_payload = 1.0;
        traceRayEXT(topLevelAS,  // acceleration structure
                    gl_RayFlagsSkipClosestHitShaderEXT,
                    0xFF,        // cullMask
                    3,           // sbtRecordOffset
                    0,           // sbtRecordStride
                    1,           // missIndex
                    position,    // ray origin
                    0.5,         // ray min range
                    light_dir,  // ray direction
                    100.0,        // ray max range
                    1            // payload (location = 1)
                    );
    }

    vec3 color = (0.7 + 0.7 * shadow_payload) * (ambient + diffuse);
    color = color + shadow_payload * spec;

    hit_value = vec4(color * vec3(0.5, 0.3, 0.1), front);
}