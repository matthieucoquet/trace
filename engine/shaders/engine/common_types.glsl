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

struct Object
{
    mat4 world_to_model;
};

struct Light
{
    vec3 position;
    vec3 color;
};

struct Material
{
    vec3 color;
};

struct Hit
{
    float dist;
    uint material_id;
};