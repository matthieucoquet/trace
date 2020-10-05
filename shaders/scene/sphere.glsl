#define ADVANCE_RATIO 0.92

float sd_sphere(in vec3 position, in float radius)
{
    float d = length(position) - radius;
    return d;
}

Hit map(in vec3 position)
{
    float d = sd_sphere(position, 0.4);
    d -= 0.01 * sin(10 * (2*scene_global.time + position.x + position.y));
    return Hit(d, RED_ID);
}
