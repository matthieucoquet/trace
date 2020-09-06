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
    vec3 ui_position;
    vec3 ui_normal;
    float time;
} scene_global;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Primitive
{
    mat4 world_to_model;
};
