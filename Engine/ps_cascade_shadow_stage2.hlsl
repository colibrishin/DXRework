#include "common.hlsli"

float GetShadowFactor(int cascadeIndex, float4 lightspacepos)
{
    float3 projCoords = lightspacepos.xyz / lightspacepos.w;
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
    const matrix world = mul(mul(scale, rotation), translation);

    matrix lightOrientedWorld[3];
	float4 cascadeLightPos[3];

    for (uint i = 0; i < MAX_NUM_CASCADES; ++i)
    {
        lightOrientedWorld[i] = lightFrustumView[i] * lightFrustumProj[i];
    }

	[unroll]
    for (int i = 0; i < 3; ++i)
    {
        cascadeLightPos[i] = mul(lightOrientedWorld[i], float4(lightFrustumPosition[i].xyz, 1.f));
    }

    float shadowFactor = 0.f;

	[unroll]
    for (int j = 0; j < MAX_NUM_CASCADES; ++j)
    {
        if (input.clipSpacePosZ <= cascadeEndClipSpace[j].z)
        {
            shadowFactor = GetShadowFactor(j, cascadeLightPos[j]);
            break;
        }
    }

    float3 shadow = lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor);

    return float4(shadow, 1.f);
}