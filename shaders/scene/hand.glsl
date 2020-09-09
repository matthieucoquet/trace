float sd_sphere(in vec3 position, in float radius)
{
    float d =  length(position) - radius;
    return d;
}

float map(in vec3 position)
{
    return sd_sphere(position, 0.4);
}
