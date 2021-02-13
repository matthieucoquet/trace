#define ADVANCE_RATIO_MISS 0.95

#define ROCK_ID 3

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

/*vec2 hash(vec2 id)
{
    float x = fract(sin(id.x * 485.1 + id.y * 2542.1) * 1254.1) - 0.5;
    float y = fract(sin(id.x * 1458.1 + id.y * 255521) * 8765.1) - 0.5;
    return vec2(x, y);
}*/

vec2 hash(vec2 p)
{
    p = vec2(dot(p,vec2(127.1,311.7)),
             dot(p,vec2(269.5,183.3)));
    return fract(sin(p)*18.5453);
}

float voronoi(in vec2 p){
    
	vec2 id = floor(p);
	vec2 pos = fract(p);

	vec2 min_dist = vec2(100.0);
    float dist = 100.0;
	for(int y = -1; y <= 1; y++){
		for(int x = -1; x <= 1; x++){
            
			vec2 off = vec2(x, y);
            off += hash(id + off) - pos;            
			dist = length(off);
            min_dist.y = max(min_dist.x, min(min_dist.y, dist));
            min_dist.x = min(min_dist.x, dist);                       
		}
	}
	
    return min_dist.y - min_dist.x;
    // return d.x;
    //return max(d.y - d.x*1.1, 0.)/.91;
    //return sqrt(d.y) - sqrt(d.x); // etc.
}


float crater(vec2 p, float r)
{
    float q = length(p);
    float d = smoothstep(0.0 , r, q);
    d = 0.5 * (pow(d, 2) - 1.0) + 0.06 * smoothstep(r * 1.4, r, q);
    return 0.6 * r * d;
}

Hit map_miss(in vec3 pos)
{
    float height = (0.0005 * length(pos.xz));
    height = height * height + 1.;

	height -= noise((pos.xz - vec2(100.0, -20.0)) * 0.011) * 12.1045;
	height += noise(pos.xz * 0.03) * 1.20;
	height += noise(pos.xz * 0.3) * 0.6045;
	height += noise(pos.xz * 3.0) * 0.01;

    {
        vec2 q = pos.xz * 0.08;
        vec2 id = floor(q);
	    q = 10. * (fract(q) - 0.5);
        vec2 center = hash(id);

	    height += crater(q - 3.0 * center, 0.8 * (center.y + 0.5));
    }

    {
        float rock = voronoi(6.0 * pos.xz);
        height += max(0, 0.1 * rock - 0.095);
		rock = voronoi(0.8 * pos.xz);
        height += max(0, 0.8 * rock - 0.8);
    }

    return Hit(pos.y - height, UNKNOW);
}

vec3 get_color_miss(in vec3 pos)
{
	vec3 color = vec3(0.45 + noise(pos.xz * 105.0) * 0.1);
	return color;
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






