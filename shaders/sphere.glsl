float sd_sphere(in vec3 position, in float radius)
{
    float d =  length(position) - radius;
    d -= 0.05 * sin(10 * (position.x + position.y));
    return d;
}

float map(in vec3 position)
{
    return sd_sphere(position, 0.9);
}

float raymarch(in Ray ray)
{
    float t = gl_RayTminEXT;
    for (int i = 0; i < 128 && t < gl_RayTmaxEXT; i++)
    {
        float distance = map(ray.origin + t * ray.direction);
        if(distance < 0.001) {
            return t;
        }
        t += distance;
    }
    return -1.0f;
}

vec3 normal(in vec3 position)
{
    vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = 0.0025;
    return normalize(
        e.xyy * map(position + e.xyy * eps).x +
        e.yyx * map(position + e.yyx * eps).x +
        e.yxy * map(position + e.yxy * eps).x +
        e.xxx * map(position + e.xxx * eps).x);
}