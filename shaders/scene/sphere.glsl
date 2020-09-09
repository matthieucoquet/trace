float sd_sphere(in vec3 position, in float radius)
{
    float d = length(position) - radius;
    return d;
}

float map(in vec3 position)
{
    float d = sd_sphere(position, 0.4);
    d -= 0.05 * sin(10 * (scene_global.time + position.x + position.y));
    return d;
}
