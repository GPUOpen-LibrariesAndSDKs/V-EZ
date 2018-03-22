#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex{
    vec4 gl_Position;
};

void main()
{
    const vec4 vertices[4] = {
        vec4(-1.0f, -1.0f, 0.0f, 1.0f),
        vec4(-1.0f, 1.0f, 0.0f, 1.0f),
        vec4(1.0f, -1.0f, 0.0f, 1.0f),
        vec4(1.0f, 1.0f, 0.0f, 1.0f)
    };

    gl_Position = vertices[gl_VertexIndex];
}