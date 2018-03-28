#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform LightMatrices
{
    mat4 viewProj;
    mat4 viewProjInv;
} light;

layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) in VS_OUT
{
    vec3 worldNormal;
    vec3 worldPos;
} ps_in;

layout(location = 0) out vec4 outColor;

void main()
{
#if 1
    vec4 shadowMapCoords = light.viewProj * vec4(ps_in.worldPos, 1.0f);
    shadowMapCoords = (shadowMapCoords / shadowMapCoords.w);
    shadowMapCoords.xy = shadowMapCoords.xy * 0.5f + 0.5f;
    float shadowMapDepth = texture(shadowMap, shadowMapCoords.xy).x;
    float shadowAttenuation = (shadowMapDepth < shadowMapCoords.z) ? 0.25f : 1.0f;
#else
    float shadowAttenuation = 1.0f;
#endif
    vec4 lightPos = light.viewProjInv * vec4(0, 0, -1, 1);
    lightPos /= lightPos.w;
    vec3 lightDir = normalize(lightPos.xyz - ps_in.worldPos);
    float nDotL = max(0.0f, dot(lightDir, ps_in.worldNormal));
    float diffuse = nDotL * shadowAttenuation;
    outColor = vec4(diffuse);    
}