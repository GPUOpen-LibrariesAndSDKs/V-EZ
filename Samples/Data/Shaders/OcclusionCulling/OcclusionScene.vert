#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout (std430, binding = 1) readonly buffer ModelDataBuffer
{
    ModelData modelData[];
};

layout(push_constant) uniform PushConstantsBlock {
    int modelIndex;
} pushConstants;

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;

layout(location = 0) out VS_OUT
{
    out vec4 eyeVertex;
    out vec4 eyeNormal;
    flat out vec4 color;
} vs_out;

void main()
{
    ModelData data = modelData[pushConstants.modelIndex];
    mat4 modelView = camera.view * data.transform;

    vs_out.eyeVertex = (modelView * vec4(in_Position, 1.0f));
    vs_out.eyeNormal = (modelView * vec4(in_Normal, 0.0f));
    vs_out.color = data.color;
    gl_Position = camera.projection * vec4(vs_out.eyeVertex.xyz, 1.0f);
}