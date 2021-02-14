#include "miss.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(location = 1) rayPayloadEXT float shadow_payload;

float ambient_occlusion_miss(in Ray ray)
{
	float occlusion = 0.0;
    float scale = 1.0;
    float len = length(ray.direction);
    for(int i = 0; i < 5; i++)
    {
        float h = 0.01 + 0.15 * float(i) / 4.0;
        float d = map_miss(ray.origin + h / len * ray.direction).dist;
        occlusion += max(0, h - d) * scale;
        scale *= 0.95;
    }
    return clamp(1.0 - 3.0 * occlusion, 0.0, 1.0);
}

float soft_shadow_miss(in Ray ray, in float factor)
{
    float res = 1.0;
    float len = length(ray.direction);
    float t = 0.03;
    for (int i = 0; i < 16; i++)
    {
        float distance = map_miss(ray.origin + t * ray.direction).dist;
        if(distance < 0.001) {
            return 0.0;
        }
        distance = distance / len;
        res = min(res, factor * distance / t);
        t += ADVANCE_RATIO_MISS * distance;
    }
    return res;
}

vec3 lighting(
    vec3 global_position,
    vec3 local_position,
    vec3 miss_position,
    vec3 global_normal,
    vec3 normal,
    mat4x3 transform,
    float scale,
    Material mat
)
{
    vec3 view_dir = normalize(transform * vec4(gl_WorldRayOriginEXT, 1.0f) - local_position);
    vec3 color = vec3(0.0);

    shadow_payload = ambient_occlusion_miss(Ray(miss_position, vec3(scene_global.transform * vec4(global_normal, 0.0))));
    traceRayEXT(topLevelAS,  // acceleration structure
        gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF,        // cullMask
        2,           // sbtRecordOffset
        0,           // sbtRecordStride
        1,           // missIndex
        global_position,    // ray origin
        0.01,         // ray min range
        global_normal,   // ray direction
        0.5,         // ray max range
        1            // payload (location = 1)
        );
    float ao = shadow_payload;

    for (int i = 0; i < scene_global.nb_lights; i++)
    {
        Light light = lights.l[nonuniformEXT(i)];
        vec3 light_dir = normalize(transform * vec4(light.position, 1.0f) - local_position);
            
        vec3 diffuse = max(dot(normal, light_dir), 0.0) * light.color;
        vec3 ambient = 0.05 * light.color;

        vec3 halfway = normalize(light_dir + view_dir);
        vec3 spec = pow(max(dot(normal, halfway), 0.0), mat.spec) * light.color;

        shadow_payload = 0.0;
        if (dot(normal, light_dir) > 0)
        {                
            light_dir = normalize(light.position - global_position);
            shadow_payload = 1.0;//soft_shadow_miss(Ray(miss_position, vec3(scene_global.transform * vec4(light_dir, 0.0))), 128.0);

            traceRayEXT(topLevelAS,  // acceleration structure
                        gl_RayFlagsSkipClosestHitShaderEXT,
                        0xFF,        // cullMask
                        1,           // sbtRecordOffset
                        0,           // sbtRecordStride
                        1,           // missIndex
                        global_position,    // ray origin
                        scale * 0.01,         // ray min range
                        light_dir,   // ray direction
                        100.0,        // ray max range
                        1            // payload (location = 1)
                        );
        }
        color = color + ambient + shadow_payload * (spec + diffuse);
    }
    return color * ao * mat.color;
}













