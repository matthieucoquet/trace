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
    vec3 color;
    float spec;
};

struct Hit
{
    float dist;
    uint material_id;
};

#define UNKNOW 10