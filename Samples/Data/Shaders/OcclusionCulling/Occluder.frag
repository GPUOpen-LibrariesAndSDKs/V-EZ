#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0f, 0.8f, 0.0f, 1.0f);
}