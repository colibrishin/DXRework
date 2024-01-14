#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
    int i = 0;

    float shadowFactor[MAX_NUM_LIGHTS];
    GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

    float4 color = input.color;

    float  lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (i = 0; i < g_lightCount.x; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));
        colorArray[i]     =
                LerpShadow(shadowFactor[i]) * bufLight[i].color * lightIntensity[i];
    }

    float4 colorSum = g_ambientColor;

    for (i = 0; i < g_lightCount.x; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    float4 finalColor = saturate(colorSum) * color;

    return finalColor;
}