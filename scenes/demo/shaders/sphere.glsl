//#define DEBUG_SDF
#define ADVANCE_RATIO 0.9

#define RED_ID 2

float sd_sphere(in vec3 position, in float radius)
{
    float d = length(position) - radius;
    return d;
}

Hit map(in vec3 position)
{
    float d = sd_sphere(position, 0.2);
    //d -= 0.01 * sin(10 * (2*scene_global.time + position.x + position.y));
    return make_hit(d, RED_ID);
}

vec3 get_color(in vec3 position)
{
    return vec3(0.0);
}