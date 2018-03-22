#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (early_fragment_tests) in;

layout (std430, binding = 2) buffer VisibilityBuffer
{
    uint visibility[];
};

layout(location = 0) in GS_OUT
{
    vec4 color;
    flat in int modelIndex;
} ps_in;

layout (location = 0) out vec4 FragColor;

void main()
{    
    visibility[ps_in.modelIndex] = 1;
    FragColor = ps_in.color;
}