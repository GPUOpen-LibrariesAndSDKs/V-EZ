#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
} ps_in;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(texSampler, ps_in.texCoord).rgb, 1.0f);
}