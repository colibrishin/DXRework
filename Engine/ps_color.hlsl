#include "common.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
    float shadowFactor[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        shadowFactor[i] = 1.0f;
    }

    [unroll]
    for (int i = 0; i < lightCount; ++i)
    {
        [unroll]
        for (int j = 0; j < MAX_NUM_CASCADES; ++j)
        {
            const matrix vp = mul(cascadeShadowChunk[i].view[j], cascadeShadowChunk[i].proj[j]);
            const float4 position = mul(input.world_position, vp);

            if (input.clipSpacePosZ <= cascadeShadowChunk[i].z_clip[j].z)
            {
                shadowFactor[i] = GetShadowFactor(i, j, position);
                break;
            }
        }
    }

	float4 color = input.color;

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));

        const float4 shadow = float4(lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor[i]), 1.f);
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
