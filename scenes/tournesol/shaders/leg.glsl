//#define DEBUG_SDF
#define ADVANCE_RATIO 0.8

#define ORANGE_ID 2

float op_union(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h); 
}

Hit map(in vec3 position)
{
    float height_ratio = smoothstep(-0.2, 0.4, position.y);
    vec3 q = position;
    q.y *= 1.5;
    q.y -= 0.8 * q.x * smoothstep(-0.7,0.7,q.y);
    q.x += 0.07 * q.y;
    vec3 c = vec3(1.0, 0.15, 1.0);
    q = q - c * clamp(round(q / c), vec3(0., -2., 0.), vec3(0., 3., 0.));
    float distance = length(q) - (0.13 + 0.04 * height_ratio);
   
    {   // Foot
        q = position;
        q.y += 0.38;
        q.x -= 0.02;
        distance = op_union(distance, length(q) - 0.1, 0.06);
        distance = op_union(distance, length(q - vec3(0.0, -0.01, 0.1)) - 0.082, 0.045);
        distance = max(distance, -position.y - 0.42);
    }
    distance = max(distance, position.y - 0.45);

    return Hit(distance, ORANGE_ID);
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 64.0);
}





