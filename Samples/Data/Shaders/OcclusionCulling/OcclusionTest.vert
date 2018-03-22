#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out VS_OUT
{
    out int modelIndex;
} vs_out;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    vs_out.modelIndex = gl_VertexIndex;
    gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}