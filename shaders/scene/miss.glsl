float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float map(in vec3 position)
{
    vec3 c = vec3(2.0, 0.0, 2.0);
    vec3 q = position - c * clamp(round(position / c), -3, 3);
    float d = sd_box(q, vec3(0.9, 0.0, 0.9)) - 0.1;
    //d = min(d, dot(position + vec3(0.0, 0.0, 20.0), vec3(0.0, 0.0, 1.0)));
    return d;
}
