//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define BLACK_ID 0
#define WHITE_ID 1
#define ORANGE_ID 2
#define RED_ID 3
#define BLUE_ID 4
#define GREY_ID 5
#define BEIGE_ID 6
#define GLASS_ID 8

layout(binding = 4, set = 0) uniform sampler2D noise_lu;

// See https://www.shadertoy.com/view/4sfGzS and iq website for more info about noise
float noise(in vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    vec2 uv = (i.xy + vec2(37.0, 17.0) * i.z) + f.xy;
    vec2 rg = textureLod(noise_lu, (uv + 0.5) / 512.0, 0.0).yx;
    return mix(rg.x, rg.y, f.z);
}

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
    Hit hit = make_hit(distance, BLUE_ID);
 
    // Red disk
    position.z = abs(position.z);
    position.z -= 0.07;
    distance = disk(position, 0.075, 0.01);
    return min_hit(hit, make_hit(distance, RED_ID));
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

Hit head(in vec3 position)
{
    float disp = 0.004 * noise(50.0 * position)
    	+ 0.0017 * noise(2500.0 * position);
    position.x = abs(position.x);
    float distance = length(position - vec3(0.0, 0.015, -0.04)) - 0.08;
	float nose = length(position - vec3(clamp(position.x, -0.02, 0.02), -0.04, 0.05)) - 0.055;
    distance = op_union(
        distance, nose,
        0.06);
    distance = op_union(
        distance, length(position - vec3(0.0, -0.02, 0.06)) - 0.045,
        0.03);
    vec3 q = position - vec3(0.05, 0.135, -0.07);
    q.xz = rotate(0.2) * q.xz;
    q.yz = rotate(-1.1) * q.yz;
    float hear = disk(q, 0.01, 0.0) - 0.02;
	distance = op_union(
        distance, hear,
        0.08);
    distance = op_union(
        distance, length(position - vec3(0.0, -0.03, -0.15)) - 0.05,
        0.06);
    distance += disp;
    Hit hit = make_hit(distance, WHITE_ID);
    
    distance = length(position - vec3(0.0, 0.01, 0.092)) - 0.016;
    distance = min(distance, length(position - vec3(0.02, 0.055, 0.02)) - 0.006);
    q = position;
    q.yz = rotate(-0.52) * q.yz;
    distance = min(distance, torus(q - vec3(0.02, 0.057, 0.038), vec2(0.0112, 0.002)));
    return min_hit(hit, make_hit(distance, BLACK_ID));;
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
    Hit hit = make_hit(distance, ORANGE_ID);
    {   // Head
        float helmet = length(position - vec3(0.0, 0.12, 0.2)) - 0.19;
        helmet = abs(helmet) - 0.0001;
        hit = min_hit(hit, Hit(helmet, GLASS_ID, 0.4));
        hit = min_hit(hit, head(position - vec3(0.0, 0.12, 0.2)));
    }
    {
        // Grey disk on belly
        distance = disk(position - vec3(0.0, 0.05, -0.15), 0.155, 0.012) - 0.005;
        hit = min_hit(hit, make_hit(distance, GREY_ID));

        // Neck
        vec3 q = position - vec3(0.0, 0.08, 0.06);
        q.yz = rotate(-0.25) * q.yz;
        distance = torus(q, vec2(0.12, 0.025));
        hit = min_hit(hit, make_hit(distance, RED_ID));
        distance = disk(q, 0.15, 0.008);
        hit = min_hit(hit, make_hit(distance, GREY_ID));
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
        hit = min_hit(hit, make_hit(distance, BEIGE_ID));
        vec3 q = position - vec3(0.0, 0.3, -0.235);
        distance = disk(q.xzy, 0.045, 0.01) - 0.007;
        hit = min_hit(hit, make_hit(distance, RED_ID));
        distance = length(q - vec3(0.06, clamp(q.y, -0.05, +0.2), 0.06)) - 0.0035;
        distance = min(
            distance, 
            length(q - vec3(0.06, clamp(q.y, -0.05, +0.11), 0.06)) - 0.006);
        hit = min_hit(hit, make_hit(distance, GREY_ID));
        distance = box(position - vec3(0.0, 0.235, -0.09), vec3(0.06, 0.035, 0.035)) - 0.003;
        hit = min_hit(hit, make_hit(distance, WHITE_ID));
    }
    return hit;
}

Material get_color(in vec3 position)
{
    return Material(vec4(0.0), 0.5, 64.0, 0.02);
}














