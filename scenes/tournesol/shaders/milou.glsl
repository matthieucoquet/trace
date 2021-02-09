//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

Hit map(in vec3 position)
{
	float dist = length(position) - 0.2;
	return Hit(dist, UNKNOW);
}

vec3 get_color(in vec3 pos)
{
	return vec3(1.0, 0.0, 0.0);
}