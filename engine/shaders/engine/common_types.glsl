struct Pose
{
    vec4 rotation;
    vec3 position;
};

struct Fov
{
    float left;
    float right;
    float up;
    float down;
};

struct Eye
{
    Pose pose;
    Fov fov;
};

layout(push_constant, scalar) uniform Scene_global {
    mat4 transform;
    Eye left;
    Eye right;
    float time;
    int nb_lights;
} scene_global;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Light
{
    vec3 local;
    vec3 position;
    vec3 color;
};

struct Material
{
    vec4 color;
    float ks;
    float shininess;
    float f0;
};

struct Hit
{
    float dist;
    uint material_id;
    float transparency;
};

Hit make_hit(in float dist, in uint material_id)
{
    return Hit(dist, material_id, 0.0);
}

#define UNKNOW 15