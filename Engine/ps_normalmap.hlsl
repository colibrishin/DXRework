#include "common.hlsli"

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    int i = 0;

    float shadowFactor[MAX_NUM_LIGHTS];
    GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

    const float4 textureColor = tex00.Sample(PSSampler, input.tex);
    float        normalLightIntensity[MAX_NUM_LIGHTS];
    float        textureLightIntensity[MAX_NUM_LIGHTS];

    float4 normalColorArray[MAX_NUM_LIGHTS];
    float4 textureColorArray[MAX_NUM_LIGHTS];

    float4 normalMap = tex01.Sample(PSSampler, input.tex);

    normalMap = (normalMap * 2.0f) - 1.0f;

    float3 bumpNormal = (normalMap.x * input.tangent) +
                        (normalMap.y * input.binormal) +
                        (normalMap.z * input.normal);
    bumpNormal = normalize(bumpNormal);

    for (i = 0; i < g_lightCount.x; ++i)
    {
        normalLightIntensity[i] =
                saturate(dot(bumpNormal, input.lightDirection[i]));
        textureLightIntensity[i] =
                saturate(dot(input.normal, input.lightDirection[i]));

        const float4 shadow = LerpShadow(shadowFactor[i]);

        normalColorArray[i]  = shadow * bufLight[i].color * normalLightIntensity[i];
        textureColorArray[i] = shadow * bufLight[i].color * textureLightIntensity[i];
    }

    float4 normalLightColor = g_ambientColor;

    for (i = 0; i < g_lightCount.x; ++i)
    {
        normalLightColor.r += normalColorArray[i].r;
        normalLightColor.g += normalColorArray[i].g;
        normalLightColor.b += normalColorArray[i].b;
    }

    float4 textureLightColor = g_ambientColor;

    for (i = 0; i < g_lightCount.x; ++i)
    {
        textureLightColor.r += textureColorArray[i].r;
        textureLightColor.g += textureColorArray[i].g;
        textureLightColor.b += textureColorArray[i].b;
    }

    float4 color =
            saturate(textureLightColor) * saturate(normalLightColor) * textureColor;

    return color;
}
