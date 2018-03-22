#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT
{
    in vec4 eyeVertex;
    in vec4 eyeNormal;
    flat in vec4 color;
} ps_in;

layout (location = 0) out vec4 FragColor;

void main()
{
    float lDotN = max(0.0f, dot(normalize(ps_in.eyeVertex.xyz), normalize(ps_in.eyeNormal.xyz)));    
    FragColor = lDotN * ps_in.color;
}