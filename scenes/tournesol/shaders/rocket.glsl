//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define WHITE_ID 1
#define RED_ID 2

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

Hit rocket(in vec3 pos)
{
	vec3 q = pos;
	float dist;
	// Body
	{
		q = pos;
		q.y += 0.8;
		float r = 0.005 * sin(2.0 * q.y * q.y * q.y);
		r += 0.05 * sin(2.0 * q.y * q.y);
		dist = length(pos.xz) - r;
		dist = max(dist, pos.y - 0.8);
	}
	{
		float antenna = length(pos - vec3(0.0, clamp(pos.y, 0.1, 0.45), 0.0)) - 0.0015;
		dist = min(antenna, dist);
	}
	// Cut under
	{
		float under = -pos.y - 0.27;
		dist = max(under, dist);
	}
	// Legs
	{
		q = pos;
		q.y += 0.41;
		float angle_sector = 6.2831 / 3.0;
		float angle_leg = round(atan(q.z, q.x) / angle_sector) * angle_sector;
		q.xz = mat2(cos(angle_leg), -sin(angle_leg), sin(angle_leg), cos(angle_leg)) * q.xz;
		dist = min(leg(q), dist);
		q = q - vec3(0.19, 0.0, 0.0);
		dist = min(feet(q), dist);
	}
	return Hit(dist, UNKNOW);
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

	return Material(abs(height-0.75) < 1.25 && pattern < 0.0 ? vec3(1.0) : vec3(0.9, 0.02, 0.02), 128.0);
}