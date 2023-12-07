#ifndef __COMMON_HLSL_
#define MAX_NUM_LIGHTS 8

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);
Texture2D shaderNormalMap : register(t1);

cbuffer PerspectiveBuffer : register(b0)
{
    matrix cam_world;
    matrix view;
    matrix projection;
};

cbuffer TransformBuffer : register(b1)
{
    matrix scale;
    matrix rotation;
    matrix translation;
};

cbuffer LightPositionBuffer : register(b2)
{
    float4 lightPosition[MAX_NUM_LIGHTS];
}

cbuffer LightColorBuffer : register(b3)
{
    float4 lightColor[MAX_NUM_LIGHTS];
}

cbuffer SpecularBuffer : register(b4)
{
    float specularPower;
    float3 _p0;
    float4 specularColor;
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