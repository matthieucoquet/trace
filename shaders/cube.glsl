float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float map(in vec3 position)
{
    float d = sd_box(position, vec3(0.4)) - 0.3;
    d -= 0.05 * pow(abs(position.y), 2.0);
    return d + 0.001;
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