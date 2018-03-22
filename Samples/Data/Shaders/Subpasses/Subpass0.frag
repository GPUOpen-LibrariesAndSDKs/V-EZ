#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT
{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
} ps_in;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 1) out vec4 redChannel;
layout(location = 2) out vec4 greenChannel;
layout(location = 3) out vec4 blueChannel;

void main()
{
    vec4 color = texture(texSampler, ps_in.texCoord);
    redChannel = vec4(color.x, 0.0f, 0.0f, 1.0f);
    greenChannel = vec4(0.0f, color.y, 0.0f, 1.0f);
    blueChannel = vec4(0.0f, 0.0f, color.z, 1.0f);
}