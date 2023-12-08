#ifndef __COMMON_HLSL_
#define MAX_NUM_LIGHTS 8

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);
Texture2D shaderNormalMap : register(t1);

Texture2DArray cascadeShadowMap : register(t2);
SamplerComparisonState shadowSampler : register(s2);

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
    matrix lightVP[MAX_NUM_LIGHTS];
    float4 lightColor[MAX_NUM_LIGHTS];
}

cbuffer SpecularBuffer : register(b3)
{
    float specularPower;
    float3 _p0;
    float4 specularColor;
}

struct GeometryInputType
{
    float2 tex : texcoord;
    float4 position : SV_POSITION;
};

struct GeometryOutputType
{
    float2 tex : texcoord;
    float4 position : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

struct VertexInputType
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;

    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;
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
#endif