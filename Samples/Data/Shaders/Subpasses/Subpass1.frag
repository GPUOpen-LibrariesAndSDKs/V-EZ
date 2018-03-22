#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(input_attachment_index = 1, set = 0, binding = 0) uniform subpassInput redChannel;
layout(input_attachment_index = 2, set = 0, binding = 1) uniform subpassInput greenChannel;
layout(input_attachment_index = 3, set = 0, binding = 2) uniform subpassInput blueChannel;

layout(location = 0) out vec4 combinedChannels;

void main()
{
    combinedChannels = subpassLoad(redChannel) + subpassLoad(greenChannel) + subpassLoad(blueChannel);
}