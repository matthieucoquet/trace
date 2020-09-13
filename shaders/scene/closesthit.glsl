layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec4 hit_value;
layout(location = 1) rayPayloadInEXT bool shadowed;
hitAttributeEXT vec3 attributes;

layout(binding = 2, set = 0, scalar) buffer Objects { Object o[]; } objects;

void main()
{
    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;  / tmax
    float front = dot(position - scene_global.ui_position, scene_global.ui_normal) <= 0.0f ? 0.0f : 1.0f;
    vec3 light_pos = vec3(10.0, 30.0, 4.0);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
  
    Object object = objects.o[gl_InstanceID];
    vec3 model_position = vec3(object.world_to_model * vec4(position, 1.0f));
    vec3 normal = normal(model_position);
    vec3 light_dir = normalize(vec3(object.world_to_model * vec4(light_pos, 1.0f)) - model_position);
    vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
    vec3 ambient = 0.2 * light_color;

    vec3 reflection = reflect(gl_WorldRayDirectionEXT, normal);
    shadowed = true;
    if (dot(normal, light_dir) > 0)
    {
        traceRayEXT(topLevelAS,
                    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
                    0xFF,        // cullMask
                    0,           // sbtRecordOffset
                    0,           // sbtRecordStride
                    1,           // missIndex
                    position,    // ray origin
                    0.1,         // ray min range
                    light_dir,   // ray direction
                    100.0,       // ray max range
                    1            // payload (for shadowed -> 1)
                    );
    }
    vec3 color = ambient + diffuse;
    if (shadowed) {
        color = 0.6 * color;
    }
    else {
        vec3 spec = hit_value.xyz * max(dot(normal, reflection), 0.0);
        color = color + spec;
    }
    hit_value = vec4(color * vec3(0.5, 0.3, 0.1), front);
}