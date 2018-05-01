#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
} ps_in;

layout(binding = 1) uniform sampler2D texSampler;

/*
layout(constant_id = 0) const float scale = 1.0f;

layout(push_constant) uniform PushConstants
{
    vec3 blendColor;
} pushConstants;
*/

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(texSampler, ps_in.texCoord);
}