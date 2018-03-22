#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform LightMatrices
{
    mat4 viewProj;
    mat4 viewProjInv;
} light;

layout(location = 0) in vec3 in_Position;

out gl_PerVertex{
    vec4 gl_Position;
};

void main()
{
    gl_Position = light.viewProj * vec4(in_Position, 1.0f);
}