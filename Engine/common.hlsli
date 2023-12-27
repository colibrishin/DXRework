#ifndef __COMMON_HLSLI__
#define TRIANGLE_MACRO 3
#include "type.hlsli"

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

Texture2D                              shaderTexture : register(t0);
Texture2D                              shaderNormalMap : register(t1);
Texture2DArray                         cascadeShadowMap[MAX_NUM_LIGHTS] : register(t2);
Texture2D                              renderedTexture : register(t3);
StructuredBuffer<BoneTransformElement> boneTransformBuffer : register(t4);

static const float4 g_ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

cbuffer PerspectiveBuffer : register(b0)
{
    matrix g_camWorld;
    matrix g_camView;
    matrix g_camProj;

    matrix g_camInvView;
    matrix g_camInvProj;

    matrix g_camReflectView;
};

cbuffer TransformBuffer : register(b1)
{
    matrix g_world;
};

cbuffer LightBuffer : register(b2)
{
    matrix g_lightWorld[MAX_NUM_LIGHTS];
    float4 g_lightColor[MAX_NUM_LIGHTS];
    int    g_lightCount;
    float3 ___p0;
}

// current light view and projection matrix of each cascade
cbuffer CascadeShadowBuffer : register(b3)
{
    CascadeShadow g_currentShadow;
}

cbuffer CascadeShadowChunk : register(b4)
{
    CascadeShadow g_cascadeShadowChunk[MAX_NUM_LIGHTS];
}

cbuffer MaterialBuffer : register(b5)
{
    float g_specularPower;
    float g_reflectionTranslation;
    float g_reflectionScale;
    float g_refractionScale;

    float4 g_overrideColor;
    float4 g_specularColor;
    float4 g_clipPlane;
}

float4 GetWorldPosition(in matrix mat)
{
    return float4(mat._14, mat._24, mat._34, mat._44);
}

float GetShadowFactorImpl(
    int    lightIndex, int cascadeIndex,
    float4 cascadeLocalPosition)
{
    float4 projCoords = cascadeLocalPosition / cascadeLocalPosition.w;
    projCoords.x      = projCoords.x * 0.5 + 0.5f;
    projCoords.y      = -projCoords.y * 0.5 + 0.5f;

    if (projCoords.z > 1.0)
    {
        return 0.f;
    }

    float currentDepth = projCoords.z;
    float bias         = 0.01f;
    float shadow       = 0.0;

    float3 samplePos = float3(projCoords.xyz);
    samplePos.z      = cascadeIndex;

    Texture2DArray shadowMap = cascadeShadowMap[lightIndex];

    [unroll] for (int x = -1; x <= 1; ++x)
    {
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            shadow += shadowMap.SampleCmpLevelZero(
                                                   PSShadowSampler, samplePos,
                                                   currentDepth - bias, int2(x, y));
        }
    }

    shadow /= 9.0f;
    return shadow;
}

void GetShadowFactor(
    in float4 world_position, in float z_clip,
    out float shadowFactor[MAX_NUM_LIGHTS])
{
    int i = 0;
    int j = 0;

    for (i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        shadowFactor[i] = 1.0f;
    }

    [unroll] for (i = 0; i < g_lightCount; ++i)
    {
        [unroll] for (j = 0; j < MAX_NUM_CASCADES; ++j)
        {
            const matrix vp = mul(
                                  g_cascadeShadowChunk[i].g_shadowView[j],
                                  g_cascadeShadowChunk[i].g_shadowProj[j]);
            const float4 position = mul(world_position, vp);

            if (z_clip <= g_cascadeShadowChunk[i].g_shadowZClip[j].z)
            {
                shadowFactor[i] = GetShadowFactorImpl(i, j, position);
                break;
            }
        }
    }
}

float4 LerpShadow(in float4 shadowFactor)
{
    return lerp(float4(0, 0, 0, 1), float4(1, 1, 1, 1), shadowFactor);
}

#endif // __COMMON_HLSLI__
