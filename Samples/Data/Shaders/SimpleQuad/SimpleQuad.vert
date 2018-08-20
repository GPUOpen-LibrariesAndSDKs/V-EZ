#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

layout(location = 0) out VS_OUT
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
} vs_out;

out gl_PerVertex{
    vec4 gl_Position;
};

void main()
{
    vs_out.pos = in_Position;
    vs_out.normal = in_Normal;
    vs_out.texCoord = in_TexCoord;    
    gl_Position = proj * view * model * vec4(in_Position, 1.0f);
}