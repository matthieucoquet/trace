//#define DEBUG_SDF
#define ADVANCE_RATIO 1.0

#define WHITE_ID 1
#define ORANGE_ID 2
#define RED_ID 3
#define BLUE_ID 4
#define GREY_ID 5
#define BEIGE_ID 6
#define LIGHT_ID 7

Hit min_hit(Hit a, Hit b)
{
    return a.dist < b.dist ? a : b;
}

float disk(in vec3 position, float radius, in float height)
{
    float distance = length(position.xz) - radius;
    vec2 w = vec2(distance, abs(position.y) - height);
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
}

float torus(vec3 position, vec2 t)
{
  vec2 q = vec2(length(position.xz) - t.x, position.y);
  return length(q) - t.y;
}

float box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit bottle(in vec3 position) {
    // Blue bottle
    float distance = length(position - vec3(0.0, clamp(position.y, -0.14, 0.14), 0.0)) - 0.06;
    distance = min(distance, length(position - vec3(0.0, clamp(position.y, 0.0, 0.22), 0.00)) - 0.0141);
    distance = min(distance, disk(position - vec3(0.0, 0.22, 0.0), 0.025, 0.006) - 0.003);
    Hit hit = Hit(distance, BLUE_ID);
 
    // Red disk
    position.y = abs(position.y);
    position.y -= 0.1;
    distance = disk(position, 0.065, 0.01);
    return min_hit(hit, Hit(distance, RED_ID));
}

Hit map(in vec3 position)
{
    // Draw orange stuff first
    float belly = disk(position, 0.135, 0.16) - 0.08;
    float distance = length(position - vec3(0.0, -0.21, 0.0)) - 0.2;
    distance = min(distance, belly);
    {
    	vec3 q = position;
        distance = min(distance, disk(q.xzy - vec3(0.0, 0.2, 0.08), 0.04, 0.012) - 0.01);
        distance = min(distance, length(q - vec3(0.0, -0.02, 0.21)) - 0.015);
        
        q = position;
        q.x = abs(q.x);
        distance = min(distance, disk(q.zxy - vec3(0.0, 0.2, 0.12), 0.06, 0.012) - 0.02);
    }
    Hit hit = Hit(distance, ORANGE_ID);
    {   // Head
        float head = length(position - vec3(0.0, 0.32, 0.0)) - 0.18;
        head = min(head, length(position - vec3(0.0, 0.08, 0.188)) - 0.05);
        hit = min_hit(hit, Hit(head, WHITE_ID));
    }
    {
        // Neck and belt
        vec3 q = position - vec3(0.0, 0.03, 0.0);        
        q.xz = q.y > 0.0 ? 1.2 * q.xz : q.xz;
        q.y = abs(q.y);
        q = q - vec3(0.0, 0.21, 0.0);
        distance = torus(q, vec2(0.19, 0.03));
        hit = min_hit(hit, Hit(distance, ORANGE_ID));
        q.z = q.z > 0.21 ? 0.99 * q.z : q.z;
        distance = disk(q, 0.22, 0.008);
        hit = min_hit(hit, Hit(distance, GREY_ID));
    }
    {
        vec3 q = position;
        distance = disk(q.xzy - vec3(0.0, 0.21, -0.1), 0.018, 0.01) - 0.004;
        q.x = abs(q.x);
        distance = min(distance, disk(q.xzy - vec3(0.07, 0.2, -0.1), 0.018, 0.01) - 0.004);
        hit = min_hit(hit, Hit(distance, LIGHT_ID));
    
    }
    {   // Bottles
        vec3 q = position;
        q.x = abs(q.x);
        hit = min_hit(hit, bottle(q - vec3(0.16, 0.0, -0.23)));
    }
    {   // Backback
        distance = box(position - vec3(0.0, 0.01, -0.25), vec3(0.08, 0.18, 0.05));
        float inv = box(position - vec3(0.0, 0.15, -0.3), vec3(0.5, 0.17, 0.045));
        distance = max(distance, -inv) - 0.01;
        hit = min_hit(hit, Hit(distance, BEIGE_ID));
        vec3 q = position - vec3(0.0, -0.09, -0.3);
        distance = disk(q.xzy, 0.03, 0.01) - 0.006;
        hit = min_hit(hit, Hit(distance, RED_ID));
        distance = length(q - vec3(0.0, clamp(q.y, 0.05, 0.51), 0.06)) - 0.002;
        distance = min(
            distance, 
            length(q - vec3(0.0, clamp(q.y, 0.05, 0.4), 0.06)) - 0.0035);
        hit = min_hit(hit, Hit(distance, GREY_ID));
        distance = box(position - vec3(0.0, 0.13, -0.23), vec3(0.06, 0.025, 0.035)) - 0.005;
        hit = min_hit(hit, Hit(distance, WHITE_ID));       
    }
    return hit;
}

Material get_color(in vec3 position)
{
    return Material(vec3(0.0), 0.5, 64.0, 0.02);
}















