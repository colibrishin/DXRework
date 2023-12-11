#ifndef __COMMON_HLSL_
#define MAX_NUM_LIGHTS 8
#define TRIANGLE_MACRO 3
#define MAX_NUM_CASCADES 3

Texture2D shaderTexture : register(t0);
Texture2D shaderNormalMap : register(t1);
Texture2DArray cascadeShadowMap : register(t2);

SamplerState PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

cbuffer PerspectiveBuffer : register(b0)
{
    matrix cam_world;
    matrix cam_view;
    matrix cam_projection;
    matrix cam_invView;
    matrix cam_invProjection;
};

cbuffer TransformBuffer : register(b1)
{
    matrix scale;
    matrix rotation;
    matrix translation;
};

cbuffer LightBuffer : register(b2)
{
    matrix lightWorld[MAX_NUM_LIGHTS];
    float4 lightColor[MAX_NUM_LIGHTS];
}

cbuffer SpecularBuffer : register(b3)
{
    float specularPower;
    float3 ___p0;
    float4 specularColor;
}

// current light view and projection matrix of each cascade
cbuffer CascadeShadowBuffer : register(b4)
{
    matrix lightFrustumView[MAX_NUM_CASCADES];
    matrix lightFrustumProj[MAX_NUM_CASCADES];
    float4 cascadeEndClipSpace[MAX_NUM_CASCADES];
}

struct VertexInputType
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;

    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;
};

struct GeometryShadowInputType
{
    float4 position : SV_POSITION;
};

struct PixelShadowStage1InputType
{
    float4 position : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;

    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;

    float3 viewDirection : TEXCOORD1;
    float3 lightDirection[MAX_NUM_LIGHTS] : TEXCOORD2;

    float clipSpacePosZ : SV_ClipDistance;
};

float4 GetWorldPosition(in matrix mat)
{
    return float4(mat._41, mat._42, mat._43, mat._44);
}

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

#endif
