#define ADVANCE_RATIO 1.0

layout(binding = 5, set = 0) uniform sampler2D ui;

float sdbox(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    float radius = 2.0;
    float height = 0.5;
    float distance = length(position.xz - vec2(0.0, radius)) - radius;
    distance = abs(distance) - 0.001;
    vec2 w = vec2(distance, abs(position.y) - height);
    distance = min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
    distance = max(distance, position.z - 0.5);
    distance = max(distance, abs(position.x) - 0.5) - 0.001;
    return Hit(distance, UNKNOW);
}

Material get_color(in vec3 position)
{
    vec2 uv = position.xy;
    uv = uv + 0.5;
    uv.y *= -1;
    return Material(textureLod(ui, nonuniformEXT(uv), 0.0).xyz, 4);
}


