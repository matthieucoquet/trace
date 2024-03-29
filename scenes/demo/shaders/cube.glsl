#define ADVANCE_RATIO 1.0

#define BLUE_ID 3

float sdbox(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    //float coeff = sin(2 * (position.y  + scene_global.time));
    //float soft = 0.1 + 0.02 * coeff * coeff;
    float d = sdbox(position, vec3(0.3));// - soft;
    return make_hit(d, BLUE_ID);
}

vec3 get_color(in vec3 position)
{
    return vec3(0.0);
}