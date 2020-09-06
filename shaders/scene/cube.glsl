float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float map(in vec3 position)
{
    float d = sd_box(position, vec3(0.3)) - 0.15;
    //d -= 0.05 * pow(abs(position.y + sin(scene_global.time * 5)), 2.0);
    return d;
}
