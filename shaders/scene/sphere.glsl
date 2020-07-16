float sd_sphere(in vec3 position, in float radius)
{
    float d =  length(position) - radius;
    d -= 0.05 * sin(10 * (scene_global.time + position.x + position.y));
    return d;
}

float map(in vec3 position)
{
    return sd_sphere(position, 0.5);
}
