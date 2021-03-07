//#define DEBUG_SDF
#define ADVANCE_RATIO 0.8

#define BLACK_ID 0
#define GLASS_ID 8
#define SKIN_ID 9
#define HAIR_ID 0

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

Hit min_hit(Hit a, Hit b)
{
    return a.dist < b.dist ? a : b;
}

mat2 rotate(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2( c, -s, s, c );
}

float disk(in vec3 position, float radius, in float height)
{
    float distance = length(position.yz) - radius;
    vec2 w = vec2(distance, abs(position.x) - height);
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
}

float torus(vec3 position, vec2 t)
{
  vec2 q = vec2(length(position.xy) - t.x, position.z);
  return length(q) - t.y;
}

float ellipsoid(vec3 position, vec3 radius)
{
  float k0 = length(position / radius);
  float k1 = length(position / (radius * radius));
  return k0 * (k0 - 1.0) / k1;
}

float op_union(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h); 
}

float hear(in vec3 position)
{
    vec3 q = position;
    q.yz = rotate(-0.2) * q.yz;
    q.xz = rotate(0.3) * q.xz;
    float distance = ellipsoid(q, vec3(0.01, 0.05, 0.04 - 0.1 * q.y));
    float inverse = length(q - vec3(0.1, 0.0, 0.0)) - 0.096;
    distance = max(distance, -inverse) - 0.0;
    return distance;
}

Hit map(in vec3 position)
{
    float helmet = length(position) - 0.45;
    helmet = abs(helmet) - 0.0001;
    Hit hit = Hit(helmet, GLASS_ID, 0.4);

    float distance = ellipsoid(position - vec3(0.0, 0.02, 0.0), vec3(0.23, 0.28, 0.25));
    distance = op_union(
        distance, ellipsoid(position - vec3(0.0, -0.1, 0.135), vec3(0.1, 0.1, 0.05)),
        0.1);
    distance = op_union(
        distance, ellipsoid(position - vec3(0.0, 0.16, 0.07), vec3(0.15, 0.1, 0.1)),
        0.1);
    
    // Hear
    vec3 q = vec3(abs(position.x), position.yz);
    distance = op_union(
        distance, hear(q - vec3(0.228, -0.012, 0.02)),
        0.005);
    // Nose
    distance = min(distance, length(position - vec3(0.0, 0., clamp(position.z, 0.0, 0.27))) - 0.06);
    hit = min_hit(hit, make_hit(distance, SKIN_ID));
    // Eyes
    distance = length(q - vec3(0.09, 0.08, 0.225)) - 0.01;    
    q.xz = rotate(-0.3) * q.xz;
    q.yz = rotate(0.05) * q.yz;
    distance = min(distance, torus(q - vec3(0.01, 0.16, 0.21), vec2(0.03, 0.004)));    
    hit = min_hit(hit, make_hit(distance, BLACK_ID));
    
    // Hair
    float disp = 0.02 * noise(50.0 * position);
    distance = ellipsoid(position - vec3(0.0, 0.05, -0.02), vec3(0.25, 0.3, 0.26));
    distance = min(distance, 
        ellipsoid(position - vec3(0.0, -0.12, 0.195), vec3(0.13, 0.13, 0.06)));    
    hit = min_hit(hit, make_hit(distance + disp, HAIR_ID));
    return hit;
}

Material get_color(in vec3 position)
{
    return Material(vec4(0.0), 0.5, 64.0, 0.02);
}


















