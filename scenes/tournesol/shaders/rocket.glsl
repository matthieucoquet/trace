//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define GREY_ID 5

float feet(in vec3 pos)
{
    pos.x = abs(pos.x);
    float small = 0.04;
    float big = 0.3;
    vec2 dir =  normalize(pos.xz) * (big - 0.045);
    return pos.y > 0.0 ? (length(pos + vec3(dir.x, -0.04, dir.y)) - big) : (length(pos) - small);
}

float leg(in vec3 pos)
{
    vec2 q = pos.xy;
    float o = length(q + vec2(0.26, 0.09)) - 0.52;
    float i = length(q + vec2(0.1, 0.09)) - 0.28;
    float d = max(o, -i);

    vec2 w = vec2(d, abs(pos.z));
    d = min(max(w.x,w.y),0.0) + length(max(w,0.0));
    return max(max(d - 0.01, pos.x - 0.2), -pos.y);
}

Hit rocket(in vec3 position)
{
    vec3 q = position;
    float distance;
    // Body
    {
        q = position;
        q.y += 0.8;
        float r = 0.005 * sin(2.0 * q.y * q.y * q.y);
        r += 0.05 * sin(2.0 * q.y * q.y);
        distance = length(position.xz) - r;
        distance = max(distance, position.y - 0.8);
    }
    {
        float antenna = length(position - vec3(0.0, clamp(position.y, 0.1, 0.45), 0.0)) - 0.0015;
        distance = min(antenna, distance);
    }
    // Cut under
    {
        float under = -position.y - 0.27;
        distance = max(under, distance);
    }
    // Legs
    {
        q = position;
        q.y += 0.41;
        float angle_sector = 6.2831 / 3.0;
        float angle_leg = round(atan(q.z, q.x) / angle_sector) * angle_sector;
        q.xz = mat2(cos(angle_leg), -sin(angle_leg), sin(angle_leg), cos(angle_leg)) * q.xz;
        distance = min(leg(q), distance);
        q = q - vec3(0.19, 0.0, 0.0);
        distance = min(feet(q), distance);
    }
    // Ladder
    {
    	q = position;
    	float angle = 0.78;
    	q.xz = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * q.xz;
    	q.z = abs(q.z); 
    	float lader = length(q - vec3(0.054, clamp(q.y, -0.48, 0.0), 0.006)) - 0.001;
    	
    	vec3 c = vec3(1.0, 0.005, 1.0);
        q = q - c * clamp(round(q / c), vec3(0., -95., 0.), vec3(0., 0., 0.));
    	float step = length(q - vec3(0.054, 0.0, clamp(q.z, 0.0, 0.006))) - 0.0005;
    	lader = min(step, lader);
    	
    	if (lader < distance){
    		return make_hit(lader, GREY_ID);
    	}
    }
    return make_hit(distance, UNKNOW);
}

Hit map(in vec3 position)
{
    return rocket(position);
}

Material get_color(in vec3 pos)
{
    float pattern = atan(pos.z, pos.x) / 6.2831 * 3.0;
    pattern = fract(pattern) - 0.5;
    float height = 8. * pos.y;
    pattern *= fract(height) - 0.5;

    return Material(abs(height-0.75) < 1.25 && pattern < 0.0 ? vec4(1.0) : vec4(0.9, 0.02, 0.02, 1.0), 0.5, 64.0, 0.2);
}



