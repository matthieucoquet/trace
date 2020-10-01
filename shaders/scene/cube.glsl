float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    float coeff = sin(2 * (position.y  + scene_global.time));
    float soft = 0.1 + 0.02 * coeff * coeff;
    float d = sd_box(position, vec3(0.3)) - soft;
    return Hit(d, BLUE_ID);
}
