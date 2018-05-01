#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    mat4 modelViewProj;
} pushConstants;

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
    gl_Position = pushConstants.modelViewProj * vec4(in_Position, 1.0f);
}