#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"
#include "sphere.glsl"

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec3 attributes;

layout(binding = 2, set = 0, scalar) buffer Primitives { Primitive p[]; } primitives;

void main()
{
	vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 light_pos = vec3(0.0, 3.0, 4.0);
	vec3 light_dir = normalize(light_pos - position);
	vec3 light_color = vec3(1.0, 1.0, 1.0);  
  
	Primitive primitive = primitives.p[gl_InstanceID];
	vec3 normal = normal(position - primitive.center);
	vec3 diffuse = max(dot(normal, light_dir), 0.0) * light_color;
	vec3 ambient = 0.2 * light_color;

	hit_value = (ambient + diffuse) * vec3(0.1, 0.4, 0.2);
}