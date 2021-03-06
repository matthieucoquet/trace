Hit raymarch(in Ray ray)
{
    float len = length(ray.direction);
    float t = gl_RayTminEXT;
    for (int i = 0; i < 128 && t < gl_RayTmaxEXT; i++)
    {
        Hit hit = map(ray.origin + t * ray.direction);
        if(hit.dist < 0.0001) {
            return Hit(t, hit.material_id, hit.transparency);
        }
        t += ADVANCE_RATIO * hit.dist / len;
    }
    return Hit(-1.0, 0, 0.0);
}

vec3 normal(in vec3 position)
{
    vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = 0.0025;
    return normalize(
        e.xyy * map(position + e.xyy * eps).dist +
        e.yyx * map(position + e.yyx * eps).dist +
        e.yxy * map(position + e.yxy * eps).dist +
        e.xxx * map(position + e.xxx * eps).dist);
}