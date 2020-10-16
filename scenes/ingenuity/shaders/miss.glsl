#define ADVANCE_RATIO 1.0

layout(binding = 2, set = 0) uniform sampler2D noise_lut;

// See https://www.shadertoy.com/view/4sfGzS and iq website for more info about noise
/*float noise(in vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    vec2 uv = (i.xy + vec2(37.0, 17.0) * i.z) + f.xy;
    vec2 rg = textureLod(noise_lut, (uv + 0.5) / 512.0, 0.0).yx;
    return mix(rg.x, rg.y, f.z);
}*/
/*vec3 noised(in vec2 x)
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
Hit map_miss(in vec3 position)
{
    vec2 pos = position.xz;
	float layer1 = noise(pos * 0.03) * 4.10 - 1.2;
	float layer2 = noise(pos * 0.53) * 0.421;
	float layer3 = noise(pos * 4.03) * 0.1045;
	float height = max(0.0, layer1 + layer2 + layer3);
    float d = position.y - height;
    //float d = position.y;

    /*float n = noise(16.0 * position);
    uint material_id = WHITE_ID;
    if (n < 0.5) {
        material_id = RED_ID;
    }*/

    return Hit(d, RED_ID);
}

vec3 background_miss(in vec3 position)
{
    return vec3(0.992,0.788,0.647);
}