#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (std140, binding = 0) uniform CameraMatrices
{    
    mat4 view;
    mat4 projection;
} camera;

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;

void main()
{
    gl_Position = camera.projection * camera.view * vec4(in_Position, 1.0f);
}