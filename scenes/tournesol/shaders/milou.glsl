//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define WHITE_ID 1
#define ORANGE_ID 2
#define RED_ID 3
#define BLUE_ID 4
#define GREY_ID 5
#define BEIGE_ID 6

mat2 rotate(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2( c, -s, s, c );
}

Hit min_hit(Hit a, Hit b)
{
    return a.dist < b.dist ? a : b;
}

float op_union(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h); 
}

float disk(in vec3 position, float radius, in float height)
{
    float distance = length(position.xy) - radius;
    vec2 w = vec2(distance, abs(position.z) - height);
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
}

float torus(vec3 position, vec2 t)
{
  vec2 q = vec2(length(position.xy) - t.x, position.z);
  return length(q) - t.y;
}

float box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit bottle(in vec3 position) {
    // Blue bottle
    float distance = length(position - vec3(0.0, 0.0, clamp(position.z, -0.1, 0.1))) - 0.07;
    distance = min(distance, length(position - vec3(0.0, 0.0, clamp(position.z, 0.0, 0.19))) - 0.015);
    distance = min(distance, disk(position - vec3(0.0, 0.0, 0.19), 0.025, 0.006) - 0.003);
    Hit hit = Hit(distance, BLUE_ID);
 
    // Red disk
    position.z = abs(position.z);
    position.z -= 0.07;
    distance = disk(position, 0.075, 0.01);
    return min_hit(hit, Hit(distance, RED_ID));
}

float leg(in vec3 position)
{
    // Leg is wider on top
    float height_ratio = smoothstep(-0.25, 0.0, position.y);
    float radius = 0.04 + 0.015 * height_ratio;
    float distance = length(position - vec3(0.0, clamp(position.y, -0.25, 0.0), 0.0)) - radius;

    {   // 4 Spheres
        vec3 q = position;
        q.y *= 1.4;
        q.y += 0.14;
        vec3 c = vec3(1.0, 0.07, 1.0);
        q = q - c * clamp(round(q / c), vec3(0., -2., 0.), vec3(0., 1., 0.));
        distance = min(distance, length(q) - (0.05 + 0.02 * height_ratio));
    }
    {   // Foot
        vec3 q = position;
        q.y += 0.28;
        q.z -= 0.025;
        distance = op_union(distance, length(q) - 0.06, 0.01);
        distance = max(distance, -position.y - 0.3);
    }
    return distance;
}

Hit map(in vec3 position)
{
    // Draw orange stuff first
    vec3 q = position;
    q.z += 0.15;
    q.x = abs(q.x);
    q.z = q.z < 0.015 ? q.z + 0.3 : q.z;
    float distance = leg(q - vec3(0.16, .0, 0.15));

    {   // Belly
        float belly = length(position - vec3(0.0, 0.05, clamp(position.z, -0.3, -0.05))) - 0.15;
        distance = min(distance, belly);
    }
    {   // Tail
        vec3 q = position - vec3(0.0, 0.15, -0.4);
        q.yz = rotate(0.6) * q.yz;
        float tail = length(q - vec3(0.0, 0.00, clamp(q.z, -0.05, +0.1))) - 0.055;
        distance = min(distance, tail);
    }
    Hit hit = Hit(distance, ORANGE_ID);
    {   // Head
        float head = length(position - vec3(0.0, 0.12, 0.2)) - 0.18;
        hit = min_hit(hit, Hit(head, WHITE_ID));
    }
    {
        // Grey disk on belly
        distance = disk(position - vec3(0.0, 0.05, -0.15), 0.155, 0.012) - 0.005;
        hit = min_hit(hit, Hit(distance, GREY_ID));

        // Neck
        vec3 q = position - vec3(0.0, 0.08, 0.06);
        q.yz = rotate(-0.25) * q.yz;
        distance = torus(q, vec2(0.12, 0.025));
        hit = min_hit(hit, Hit(distance, RED_ID));
        distance = disk(q, 0.15, 0.008);
        hit = min_hit(hit, Hit(distance, GREY_ID));
    }
    {   // Bottles
        vec3 q = position;
        q.x = abs(q.x);
        hit = min_hit(hit, bottle(q - vec3(0.12, 0.2, -0.18)));
    }
    {   // Backback
        distance = box(position - vec3(0.0, 0.24, -0.17), vec3(0.07, 0.044, 0.14));
        float inv = box(position - vec3(0.0, 0.3, 0.0), vec3(0.5, 0.045, 0.17));
        distance = max(distance, -inv) - 0.01;
        hit = min_hit(hit, Hit(distance, BEIGE_ID));
        vec3 q = position - vec3(0.0, 0.3, -0.235);
        distance = disk(q.xzy, 0.045, 0.01) - 0.007;
        hit = min_hit(hit, Hit(distance, RED_ID));
        distance = length(q - vec3(0.06, clamp(q.y, -0.05, +0.2), 0.06)) - 0.0035;
        distance = min(
            distance, 
            length(q - vec3(0.06, clamp(q.y, -0.05, +0.11), 0.06)) - 0.006);
        hit = min_hit(hit, Hit(distance, GREY_ID));
        distance = box(position - vec3(0.0, 0.235, -0.09), vec3(0.06, 0.035, 0.035)) - 0.003;
        hit = min_hit(hit, Hit(distance, WHITE_ID));        
    }
    return hit;
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 64.0);
}
