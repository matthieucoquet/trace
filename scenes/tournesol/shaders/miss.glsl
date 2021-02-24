#define ADVANCE_RATIO_MISS 0.95

#define ROCK_ID 3
//#define GO_FAST 

layout(binding = 3, set = 0, scalar) buffer Lights { Light l[]; } lights;
layout(binding = 4, set = 0) uniform sampler2D noise_lut;
layout(binding = 6, set = 0) uniform sampler2D earth_sampler;

// See https://www.shadertoy.com/view/4sfGzS and iq website for more info about noise
float noise(in vec2 x)
{
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);
    vec2 p = floor(x);
    float a = textureLod(noise_lut, nonuniformEXT((p + vec2(0.5,0.5)) / 512.0), 0.0).x;
    float b = textureLod(noise_lut, nonuniformEXT((p + vec2(1.5,0.5)) / 512.0), 0.0).x;
    float c = textureLod(noise_lut, nonuniformEXT((p + vec2(0.5,1.5)) / 512.0), 0.0).x;
    float d = textureLod(noise_lut, nonuniformEXT((p + vec2(1.5,1.5)) / 512.0), 0.0).x;
    return a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y;
}

vec2 hash(in vec2 position)
{
    position = vec2(dot(position, vec2(127.1, 311.7)),
             dot(position, vec2(269.5, 183.3)));
    return fract(sin(position) * 18.5453);
}

// Return first, second and hash
vec4 voronoi(in vec2 position)
{
    vec2 id = floor(position);
    vec2 pos = fract(position);

    vec4 min_dist = vec4(100.0);
    float dist = 100.0;
    for(int y = -1; y <= 1; y++){
        for(int x = -1; x <= 1; x++){
            vec2 off = vec2(x, y);
            vec2 r = hash(id + off);
            off += r - pos;
            dist = length(off);
            min_dist.y = max(min_dist.x, min(min_dist.y, dist));
            if (min_dist.x > dist) {
                min_dist.x = min(min_dist.x, dist);
                min_dist.zw = r;
            }
        }
    }
    return min_dist;
}
float voronoi2(in vec2 position){
    vec4 min_dist = voronoi(position);
    return min_dist.y - min_dist.x;
}

float crater(vec2 position)
{
    float scale = 0.04;
    vec4 v = voronoi(scale * position);
    float q = v.x / scale;
    float radius = 0.5 + 1.5 * v.z;
    float distance = smoothstep(0.0, radius, q);
    distance = 0.2 * (pow(distance, 2) - 1.0) + 0.03 * smoothstep(radius * 1.4, radius, q);
    return distance * radius;
}

Hit map_miss(in vec3 position)
{
    // Curvature of moon
    float height = (0.0008 * length(position.xz));
    height = height * height + 5.13;

    // Use to discard some elements
    float dist2 = dot(position.xz, position.xz);

	vec2 q = position.xz - vec2(-630.0, -535.0);
    height -= noise(q * 0.011) * 20.1045;
    
    height += noise(q * 0.03) * 7.20;
    height += noise(q * 0.113) * 1.18045;
    height += noise(q * 0.283) * 0.6055;
#ifndef GO_FAST
    if (dist2 < 1000) {
        height += noise(q * 1.) * 0.094821;
    }
    if (dist2 < 100000)
    {
        height += crater(position.xz);
    }
    if (dist2 < 10000.0)
    {
        float rock = voronoi2(6.0 * position.xz);
        height += max(0, 0.1 * rock - 0.095);
        rock = voronoi2(0.8 * position.xz);
        height += max(0, 0.8 * rock - 0.8);
    }
#endif
    return Hit(position.y - height, UNKNOW);
}

Material get_color_miss(in vec3 position)
{
    vec3 color = vec3(0.1 + noise(position.xz * 105.0) * 0.02);
    return Material(color, 4.0);
}

vec3 background_miss(in vec3 direction)
{
    Light light = lights.l[0];
    vec3 light_dir = normalize(vec3(scene_global.transform * vec4(light.position, 1.0f)));
    float sun = 0.0000005 / pow(1.0 - dot(light_dir, direction), 2);
    sun = min(sun, 1.0);

    vec2 proj = direction.yz * 1000.0 / (-direction.x);
    proj = (proj + vec2(-200.0, 16.0)) / 32.0;
    proj = clamp(proj, 0.0, 1.0);
    vec4 earth = texture(earth_sampler, proj);
    proj = proj - 0.5;
    vec3 norm = vec3(sqrt(max(0.0, 0.25 - proj.x * proj.x - proj.y * proj.y)), proj.xy);
    vec3 col = sun * vec3(1.0);
    if (dot(norm, light_dir) > 0.0){
        col = mix(col, earth.rgb, earth.a);
    }
    return col;
}
























