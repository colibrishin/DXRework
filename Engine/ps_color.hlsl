#include "common.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
    int i = 0;

    float shadowFactor[MAX_NUM_LIGHTS];
    GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

	float4 color = input.color;

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));
        colorArray[i] = LerpShadow(shadowFactor[i]) * g_lightColor[i] * lightIntensity[i];
    }

    float4 colorSum = g_ambientColor;

    for (i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    float4 finalColor = saturate(colorSum) * color;

    return finalColor;
}
