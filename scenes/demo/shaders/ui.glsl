#define ADVANCE_RATIO 1.0

layout(binding = 5, set = 0) uniform sampler2D ui;

float sdbox(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    float d = sdbox(position, vec3(0.5, 0.5, 0.01));
    return Hit(d, UNKNOW);
}

vec3 get_color(in vec3 position)
{
    vec2 uv = position.xy;
    uv = uv + 0.5;
    uv.y *= -1;
    return textureLod(ui, nonuniformEXT(uv), 0.0).xyz;
}