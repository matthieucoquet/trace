#define ADVANCE_RATIO 0.9

#define ORANGE_ID 2

float op_union(float d1, float d2, float k)
{
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h); 
}

float box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float hand(in vec3 position)
{
	float distance = box(position, vec3(0.06, 0.03, 0.0)) - 0.03;
	
	vec3 q = position;
	q.y -= 0.01;
	vec3 c = vec3(1.0, 0.025, 1.0);
    q = q - c * clamp(round(q / c), vec3(0., -2., 0.), vec3(0., 1., 0.));
    float finger = length(q - vec3(clamp(q.x, 0.05, 0.2), 0.0, 0.0)) - 0.011;
	
	distance = op_union(distance, finger, 0.02);
	return distance;	
}

Hit map(in vec3 position)
{
    vec3 q = position;
    // Leg is wider on top
    float height_ratio = smoothstep(0.2, -0.4, position.x);
    float radius = 0.06 + 0.04 * height_ratio;
    float distance = length(q - vec3(clamp(q.x, -0.4, 0.1), 0.0, 0.0)) - radius;

    {   // 4 Spheres
        q.x *= 1.4;
        q.x += 0.0;
        
        vec3 c = vec3(0.07, 1.0, 1.0);
        q = q - c * clamp(round(q / c), vec3(-2., 0., 0.), vec3(1., 0., 0.));
        distance = min(distance, length(q) - (0.07 + 0.05 * height_ratio));
    }
    
    distance = op_union(distance, hand(position - vec3(0.2, 0.0, 0.0)), 0.03);
    return Hit(distance, ORANGE_ID);
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 64.0);
}




