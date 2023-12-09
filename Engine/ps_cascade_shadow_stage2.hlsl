#include "common.hlsli"

float GetShadowFactor(int cascadeIndex, float4 cascadeLocalPosition)
{
    float3 projCoords = cascadeLocalPosition.xyz / cascadeLocalPosition.w;
    projCoords.x = projCoords.x * 0.5 + 0.5f;
    projCoords.y = -projCoords.y * 0.5 + 0.5f;

    if (projCoords.z > 1.0)
    {
        return 0.f;
    }

    float currentDepth = projCoords.z;
    float bias = 0.01f;
    float shadow = 0.0;

    float3 samplePos = projCoords;
    samplePos.z = cascadeIndex;

	[unroll]
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            shadow += cascadeShadowMap.SampleCmpLevelZero(PSShadowSampler, samplePos, currentDepth - bias, int2(x, y));
        }
    }

    shadow /= 9.0f;
    return shadow;
}


float4 main(PixelShadowStage2InputType input) : SV_TARGET
{
	float4 cascadeLocalPosition[3];

	[unroll]
    for (int i = 0; i < 3; ++i)
    {
        cascadeLocalPosition[i] = input.position;
        cascadeLocalPosition[i] = mul(cascadeLocalPosition[i], lightFrustumView[i]);
        cascadeLocalPosition[i] = mul(cascadeLocalPosition[i], lightFrustumProj[i]);
    }

    float shadowFactor = 0.f;

	[unroll]
    for (int j = 0; j < MAX_NUM_CASCADES; ++j)
    {
        if (input.clipSpacePosZ <= cascadeEndClipSpace[j].z)
        {
            shadowFactor = GetShadowFactor(j, cascadeLocalPosition[j]);
            break;
        }
    }

    float3 shadow = lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor);

    return float4(shadow, 1.f);
}