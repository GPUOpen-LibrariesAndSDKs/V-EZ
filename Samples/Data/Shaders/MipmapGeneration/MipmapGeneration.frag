#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
} ps_in;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float mipLevel;
};

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = textureLod(texSampler, ps_in.texCoord, mipLevel);
}