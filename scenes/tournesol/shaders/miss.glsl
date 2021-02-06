#define ADVANCE_RATIO_MISS 0.9

#define ROCK_ID 3

layout(binding = 4, set = 0) uniform sampler2D noise_lut;

// See https://www.shadertoy.com/view/4sfGzS and iq website for more info about noise
/*float noise(in vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    vec2 uv = (i.xy + vec2(37.0, 17.0) * i.z) + f.xy;
    vec2 rg = textureLod(noise_lut, (uv + 0.5) / 512.0, 0.0).yx;
    return mix(rg.x, rg.y, f.z);
}
vec3 noised(in vec2 x)
{
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);

    vec2 p = floor(x);
    float a = textureLod(noise_lut, nonuniformEXT((p + vec2(0.5,0.5)) / 512.0), 0.0).x;
    float b = textureLod(noise_lut, nonuniformEXT((p + vec2(1.5,0.5)) / 512.0), 0.0).x;
    float c = textureLod(noise_lut, nonuniformEXT((p + vec2(0.5,1.5)) / 512.0), 0.0).x;
    float d = textureLod(noise_lut, nonuniformEXT((p + vec2(1.5,1.5)) / 512.0), 0.0).x;

    return vec3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
                6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));
}*/
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
/*
const mat2 m2 = mat2(  0.80,  0.60,
                      -0.60,  0.80 );
float fbm(in vec2 pos)
{
    float frequ = 1.9;
    float scale = 0.55;
    float ampl = 0.0;
    float ampl_factor = 0.5;
    for( int i=0; i<9; i++ )
    {
        float n = noised(pos).x;
        ampl += ampl_factor * n;
        ampl_factor *= scale;
        pos = frequ * m2 * pos;
    }
    return ampl;
}


*/

vec2 hash(vec2 id)
{
    float x = fract(sin(id.x * 485.1 + id.y * 2542.1) * 1254.1) - 0.5;
    float y = fract(sin(id.x * 1458.1 + id.y * 255521) * 8765.1) - 0.5;
    return vec2(x, y);
}

float crater(vec2 p, float r)
{
    float q = length(p);
    float d = smoothstep(0.0 , r, q);
    d = 0.5 * (pow(d, 2) - 1.0) + 0.03 * smoothstep(r * 1.4, r, q);
    return r * d;
}

Hit map_miss(in vec3 pos)
{
    float height = (0.0005 * length(pos.xz));
    height = height * height;

	height -= noise(pos.xz * 0.01) * 17.1045;
	height += noise(pos.xz * 3.0) * 0.02;

    vec2 q = pos.xz * 0.05;
    vec2 id = floor(q);
	q = 20. * (fract(q) - 0.5);
    vec2 center = hash(id);

	height += crater(q - 6.0 * center, 4.0 * (center.y + 0.5));

    return Hit(pos.y - height, ROCK_ID);
}

vec3 background_miss(in vec3 position)
{
    return vec3(0.0,0.0,0.0);
}
