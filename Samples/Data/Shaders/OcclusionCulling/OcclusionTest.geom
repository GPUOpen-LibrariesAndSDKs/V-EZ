#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

struct ModelData
{
    mat4 transform;
    vec4 color;
    vec4 bboxMin;
    vec4 bboxMax;
};

layout (std140, binding = 0) uniform CameraMatrices
{
    mat4 view;
    mat4 projection;
} camera;

layout (std430, binding = 1) buffer ModelDataBuffer
{
    ModelData modelData[];
};

layout(location = 0) in VS_OUT
{
    in int modelIndex;
} gs_in[];

layout(location = 0) out GS_OUT
{
    vec4 color;
    flat out int modelIndex;
} gs_out;

void main()
{
    // Get current model's data out of the SSBO.
    ModelData data = modelData[gs_in[0].modelIndex];

    // Multiply all matrices once.
    mat4 modelView = camera.view * data.transform;
    mat4 modelViewProjection = camera.projection * modelView;

    // ModelID remains the same for every output primitive.
    gs_out.modelIndex = gs_in[0].modelIndex;
    gs_out.color = data.color;

    // Output the back face if its oriented towards the eye point.
    vec4 eyeSpaceNormal = modelView * vec4(0.0f, 0.0f, -1.0f, 0.0f);
    vec4 eyeSpaceVertex = modelView * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }

    
    // Output the front face if its oriented towards the eye point.
    eyeSpaceNormal = modelView * vec4(0.0f, 0.0f, 1.0f, 0.0f);
    eyeSpaceVertex = modelView * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }

    
    // Output the left face if its oriented towards the eye point.
    eyeSpaceNormal = modelView * vec4(-1.0f, 0.0f, 0.0f, 0.0f);
    eyeSpaceVertex = modelView * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }
    
    // Output the right face if its oriented towards the eye point.
    eyeSpaceNormal = modelView * vec4(1.0f, 0.0f, 0.0f, 0.0f);
    eyeSpaceVertex = modelView * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }
    
    // Output the bottom face if its oriented towards the eye point.
    eyeSpaceNormal = modelView * vec4(0.0f, -1.0f, 0.0f, 0.0f);
    eyeSpaceVertex = modelView * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMax.x, data.bboxMin.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }

    // Output the top face if its oriented towards the eye point.
    eyeSpaceNormal = modelView * vec4(0.0f, 1.0f, 0.0f, 0.0f);
    eyeSpaceVertex = modelView * vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
    if (dot(normalize(eyeSpaceNormal.xyz), normalize(-eyeSpaceVertex.xyz)) > 0)
    {
        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMin.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection * vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMax.z, 1.0f);
        EmitVertex();

        gl_Position = modelViewProjection *  vec4(data.bboxMax.x, data.bboxMax.y, data.bboxMin.z, 1.0f);
        EmitVertex();

        EndPrimitive();
    }
}