#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraMatrices
{
    mat4 viewProj;
    mat4 viewProjInv;
} camera;

layout(binding = 1) uniform LightMatrices
{
    mat4 viewProj;
    mat4 viewProjInv;
} light;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

layout(location = 0) out VS_OUT
{
    vec3 worldNormal;
    vec3 worldPos;
} vs_out;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vs_out.worldNormal = in_Normal;
    vs_out.worldPos = in_Position;
    gl_Position = camera.viewProj * vec4(vs_out.worldPos, 1.0f);
}