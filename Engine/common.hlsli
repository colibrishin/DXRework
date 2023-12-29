#ifndef __COMMON_HLSLI__
#define TRIANGLE_MACRO 3
#include "type.hlsli"

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

Texture2D      tex00 : register(t0);
Texture2D      tex01 : register(t1);
Texture2D      tex02 : register(t2);
Texture2D      tex03 : register(t3);
Texture2D      tex04 : register(t4);
Texture2D      tex05 : register(t5);
Texture2D      tex06 : register(t6);
Texture2D      tex07 : register(t7);
Texture2DArray texArr00 : register(t8);
Texture2DArray texArr01 : register(t9);
Texture2DArray texArr02 : register(t10);
Texture2DArray texArr03 : register(t11);
Texture2DArray texArr04 : register(t12);
Texture2DArray texArr05 : register(t13);
Texture2DArray texArr06 : register(t14);
Texture2DArray texArr07 : register(t15);
TextureCube    texCube00 : register(t16);
TextureCube    texCube01 : register(t17);
TextureCube    texCube02 : register(t18);
TextureCube    texCube03 : register(t19);
TextureCube    texCube04 : register(t20);
TextureCube    texCube05 : register(t21);
TextureCube    texCube06 : register(t22);
TextureCube    texCube07 : register(t23);

Texture2DArray                         texShadowMap[MAX_NUM_LIGHTS] : register(t64);
Texture2D                              texRendered : register(t65);
StructuredBuffer<BoneTransformElement> bufBoneTransform : register(t66);

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
    return float4(mat._41, mat._42, mat._43, mat._44);
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

    Texture2DArray shadowMap = texShadowMap[lightIndex];

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
