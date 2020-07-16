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

layout(binding = 3, scalar) uniform Scene_global
{
    Eye left;
    Eye right;
    float time;
} scene_global;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Primitive
{
    vec3 center;
};
