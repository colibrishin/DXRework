#include "common.hlsli"

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
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

    const float4 textureColor = shaderTexture.Sample(PSSampler, input.tex);
    float normalLightIntensity[MAX_NUM_LIGHTS];
    float textureLightIntensity[MAX_NUM_LIGHTS];

    const float4 ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

    float4 normalColorArray[MAX_NUM_LIGHTS];
    float4 textureColorArray[MAX_NUM_LIGHTS];

    float4 normalMap = shaderNormalMap.Sample(PSSampler, input.tex);

    normalMap = (normalMap * 2.0f) - 1.0f;

    float3 bumpNormal = (normalMap.x * input.tangent) + (normalMap.y * input.binormal) + (normalMap.z * input.normal);
    bumpNormal = normalize(bumpNormal);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        normalLightIntensity[i] = saturate(dot(bumpNormal, input.lightDirection[i]));
        textureLightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));

        const float4 shadow = float4(lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor[i]), 1.f);

        normalColorArray[i] = shadow * lightColor[i] * normalLightIntensity[i];
        textureColorArray[i] = shadow * lightColor[i] * textureLightIntensity[i];
    }

    float4 normalLightColor = ambientColor;

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        normalLightColor.r += normalColorArray[i].r;
        normalLightColor.g += normalColorArray[i].g;
        normalLightColor.b += normalColorArray[i].b;
    }

    float4 textureLightColor = ambientColor;

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        textureLightColor.r += textureColorArray[i].r;
        textureLightColor.g += textureColorArray[i].g;
        textureLightColor.b += textureColorArray[i].b;
    }

    float4 color = saturate(textureLightColor) * saturate(normalLightColor) * textureColor;

    return color;
}
