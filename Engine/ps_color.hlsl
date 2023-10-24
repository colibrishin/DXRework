//////////////
// TYPEDEFS //
//////////////
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);

#define MAX_NUM_LIGHTS 8

cbuffer LightColorBuffer : register(b3)
{
    float4 lightColor[MAX_NUM_LIGHTS];
}

struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;
    float3 lightPos[MAX_NUM_LIGHTS] : TEXCOORD1;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    float4 color = input.color;

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightPos[i]));
        colorArray[i] = lightColor[i] * lightIntensity[i];
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
