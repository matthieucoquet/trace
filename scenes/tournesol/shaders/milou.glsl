//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define WHITE_ID 1
#define RED_ID 2
#define BLUE_ID 3
#define GREY_ID 4
#define BEIGE_ID 5

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

float sdDisk(in vec3 pos, float rad, in float h)
{
	vec2 q = pos.xy;
	float dist = length(q) - rad;

	vec2 w = vec2(dist, abs(pos.z) - h);
	return min(max(w.x,w.y),0.0) + length(max(w,0.0));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xy) - t.x, p.z);
  return length(q) - t.y;
}

float sdbox(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit bottle(in vec3 pos) {
    float dist = length(pos - vec3(0.0, 0.0, clamp(pos.z, -0.1, 0.1))) - 0.07;
    Hit hit = Hit(dist, BLUE_ID);
 
	pos.z = abs(pos.z);
	pos.z -= 0.07;
	dist = sdDisk(pos - vec3(0.0, 0.0, 0.0), 0.075, 0.01);

    return min_hit(hit, Hit(dist, RED_ID));
}

float leg(in vec3 pos)
{
    float height_ratio = smoothstep(-0.25, 0.0, pos.y);
    float rad = 0.04 + 0.015 * height_ratio;
    float dist = length(pos - vec3(0.0, clamp(pos.y, -0.25, 0.0), 0.0)) - rad;

    {
        vec3 q = pos;
        q.y *= 1.4;
        q.y += 0.14; 
    
        vec3 c = vec3(1.0, 0.07, 1.0);
        q = q - c * clamp(round(q / c), vec3(0., -2., 0.), vec3(0., 1., 0.));
        dist = min(dist, length(q) - (0.05 + 0.02 * height_ratio));
    }
    {
        vec3 q = pos;
        q.y += 0.28;
        q.z -= 0.025;
        dist = op_union(dist, length(q) - 0.06, 0.01);
        dist = max(dist, -pos.y - 0.3);
    }

    return dist;
}

Hit map(in vec3 pos)
{
    // Draw red stuff first
    vec3 q = pos;
    q.z += 0.15;
    q.x = abs(q.x);
    q.z = q.z < 0.015 ? q.z + 0.3 : q.z;
    float dist = leg(q - vec3(0.16, .0, 0.15));

    {
        float dist_belly = length(pos - vec3(0.0, 0.05, clamp(pos.z, -0.3, -0.05))) - 0.15;
        dist = min(dist, dist_belly);
    }
    {
		vec3 q = pos- vec3(0.0, 0.15, -0.4);
        q.yz = rotate(0.6) * q.yz;
        float tail = length(q - vec3(0.0, 0.00, clamp(q.z, -0.05, +0.1))) - 0.055;
        dist = min(dist, tail);
    }
    Hit hit = Hit(dist, RED_ID);

    {
        float dist_head = length(pos - vec3(0.0, 0.12, 0.2)) - 0.18;
        hit = min_hit(hit, Hit(dist_head, WHITE_ID));
    }
    {
		dist = sdDisk(pos - vec3(0.0, 0.05, -0.15), 0.155, 0.012) - 0.005;
        hit = min_hit(hit, Hit(dist, GREY_ID));

        vec3 q = pos - vec3(0.0, 0.08, 0.06);
        q.yz = rotate(-0.25) * q.yz;
        dist = sdTorus(q, vec2(0.12, 0.025));
        hit = min_hit(hit, Hit(dist, RED_ID));

		dist = sdDisk(q, 0.15, 0.008);
        hit = min_hit(hit, Hit(dist, GREY_ID));
    }
    {
        vec3 q = pos;
        q.x = abs(q.x);
        hit = min_hit(hit, bottle(q - vec3(0.12, 0.2, -0.18)));
    }
	{
		dist = sdbox(pos - vec3(0.0, 0.24, -0.17), vec3(0.07, 0.044, 0.14));
		float inv = sdbox(pos - vec3(0.0, 0.3, 0.0), vec3(0.5, 0.045, 0.17));
		dist = max(dist, -inv) - 0.01;
        hit = min_hit(hit, Hit(dist, BEIGE_ID));
	}

    return hit;
}

vec3 get_color(in vec3 pos)
{
    return vec3(1.0, 0.0, 0.0);
}