#define ADVANCE_RATIO 0.9

#define BLACK_ID 0
#define RED_ID 2

float disk(in vec3 pos, float rad, in float h)
{
    vec2 q = pos.xy;
    float dist = length(q) - rad;
    vec2 w = vec2(dist, abs(pos.z) - h);
    return min(max(w.x,w.y),0.0) + length(max(w,0.0));
}

float capsule(vec3 position, float height, float radius)
{
    position.z -= clamp(position.z, 0.0, height);
    return length(position) - radius;
}

vec2 rotate(vec2 position, float angle)
{
    mat2 rot = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    return rot * position;
}

float op_union(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h); 
}

Hit map(in vec3 position)
{
    vec3 ring_pos = position;
    ring_pos.y -= 0.2;
    ring_pos.yz = rotate(ring_pos.yz, -0.6);
    float outer = length(ring_pos) - 0.3;
    float inner = length(ring_pos.xy) - 0.28;
    float ring = max(outer, -inner);

    vec3 holder_pos = position;
    holder_pos.yz = rotate(holder_pos.yz, -0.65);
    float hold = capsule(holder_pos, 0.4, 0.1) - 0.08 * smoothstep(0.4, -0.2, position.z);

    hold = max(hold, position.y);
    
    hold = min(hold, disk(position.xzy - vec3(-0.04, 0.07, 0.0), 0.04, 0.02) - 0.01);
    hold = min(hold, disk(position.xzy - vec3(-0.08, -0.05, 0.0), 0.04, 0.02) - 0.01);
    hold = min(hold, disk(position.xzy - vec3(0.06, -0.05, 0.0), 0.01, 0.03) - 0.01);
    hold = min(hold, disk(position.xzy - vec3(0.06, -0.05, 0.04), 0.05, 0.005) - 0.01);

    uint material_id = hold - 0.01 < ring ? BLACK_ID : RED_ID;
    float d = op_union(hold, ring, 0.08);
    return Hit(d, material_id);
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 64.0);
}
