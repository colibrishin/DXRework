#ifndef __COMMON_HLSL_
#define MAX_NUM_LIGHTS 8
#define TRIANGLE_MACRO 3
#define MAX_NUM_CASCADES 3

Texture2D shaderTexture : register(t0);
Texture2D shaderNormalMap : register(t1);
Texture2DArray cascadeShadowMap : register(t2);

SamplerState PSSampler : register(s1);
SamplerComparisonState PSShadowSampler : register(s1);

cbuffer PerspectiveBuffer : register(b0)
{
    matrix cam_world;
    matrix cam_view;
    matrix cam_projection;
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
    float4 lightFrustumPosition[MAX_NUM_CASCADES];
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

    float clipSpacePosZ : SV_ClipDistance;
};

struct GeometryShadowInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelShadowStage1InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

struct PixelShadowStage2InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float clipSpacePosZ : SV_ClipDistance;
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
};

float4 GetWorldPosition(in matrix mat)
{
    return float4(mat._41, mat._42, mat._43, mat._44);
}

#endif
