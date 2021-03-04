#define ADVANCE_RATIO 0.9

#define ORANGE_ID 2

mat2 rotate(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2( c, -s, s, c );
}

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

float finger(in vec3 position, float angle, float len, float radius)
{
	vec3 q = position;
	q.x -= 0.08;
	q.xz = rotate(angle) * q.xz;
	float distance = length(q - vec3(clamp(q.x, 0.0, 0.05 * len), 0.0, 0.0)) - 0.016 * radius;
	q.x -= 0.05 * len;
	q.xz = rotate(angle) * q.xz;
	distance = op_union(distance, 
		length(q - vec3(clamp(q.x, 0.0, 0.04 * len), 0.0, 0.0)) - 0.0145 * radius,
		0.003);
	q.x -= 0.04 * len;
	q.xz = rotate(angle) * q.xz;
	distance = op_union(distance, 
		length(q - vec3(clamp(q.x, 0.0, 0.03 * len), 0.0, 0.0)) - 0.0145 * radius,
		0.003);
	return distance;
}

float hand(in vec3 position)
{
	vec3 q = position;
	q.z -= abs(q.y) * 0.15;
	float distance = box(q, vec3(0.06, 0.03, 0.005)) - 0.02;
	
	q.y += 0.016;
	/*vec3 c = vec3(1.0, 0.025, 1.0);
    q = q - c * clamp(round(q / c), vec3(0., -2., 0.), vec3(0., 1., 0.));*/
    float k = 0.005;
    q.xy = rotate(-0.2) * q.xy;
    distance = op_union(distance, finger(q, 0.3, 0.85, 0.95), k);
    q.xy = rotate(0.17) * q.xy;
	q.y -= 0.01;
    distance = op_union(distance, finger(q, 0.3, 0.93, 0.93), k);
    q.xy = rotate(0.15) * q.xy;
	q.y -= 0.01;
    distance = op_union(distance, finger(q, 0.35, 1.0, 0.95), k);
    q.xy = rotate(0.15) * q.xy;
	q.y -= 0.01;
    distance = op_union(distance, finger(q, 0.4, 0.93, 0.93), k);
    
    q.xy = rotate(0.6) * q.xy;
    q.yz = rotate(0.5) * q.yz;
    q.xz = rotate(0.1) * q.xz;
    q += vec3(0.05, -0.005, 0.0);
    distance = op_union(distance, finger(q, 0.4, 0.7, 1.0), 0.02);
	
	return distance;	
}

Hit map(in vec3 position)
{
    vec3 q = position;
      
    float height_ratio = smoothstep(0.2, -0.4, position.x);
    float radius = 0.06 + 0.04 * height_ratio;
    float angle = 0.4;
    float distance = length(q - vec3(clamp(q.x, -0.4, 0.), 0.0, 0.0)) - radius;
    {   // 4 Spheres
    	q.xy = rotate(angle * smoothstep(-0.15, 0.1, position.x)) * q.xy;
        q.x *= 1.4;
        q.x += 0.0;
        
        vec3 c = vec3(0.07, 1.0, 1.0);
        q = q - c * clamp(round(q / c), vec3(-2., 0., 0.), vec3(1., 0., 0.));
        distance = min(distance, length(q) - (0.07 + 0.05 * height_ratio));
    }
    q = position;
    q.xy = rotate(angle) * q.xy;
    distance = min(distance, length(q - vec3(clamp(q.x, 0.0, 0.1), 0.0, 0.0)) - radius);
    distance = op_union(distance, hand(q - vec3(0.2, 0.0, 0.0)), 0.03);
    return Hit(distance, ORANGE_ID);
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 0.5, 64.0, 0.02);
}











