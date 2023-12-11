#include "common.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
    float4 cascadeLocalPosition[3];

    for (int i = 0; i < 3; ++i)
    {
        cascadeLocalPosition[i] = mul(input.world_position, mul(lightFrustumView[i], lightFrustumProj[i]));
    }

    float shadowFactor = 0.f;

    for (int j = 0; j < MAX_NUM_CASCADES; ++j)
    {
        if (input.clipSpacePosZ <= cascadeEndClipSpace[j].z)
        {
            shadowFactor = GetShadowFactor(j, cascadeLocalPosition[j]);
            break;
        }
    }

    const float4 shadow = float4(lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor), 1.f);

    float4 color = input.color;

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));
        colorArray[i] = shadow * lightColor[i] * lightIntensity[i];
    }

    float4 colorSum = float4(0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    float4 finalColor = saturate(colorSum) * color;

    return finalColor;
}
