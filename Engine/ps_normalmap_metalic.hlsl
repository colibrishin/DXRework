//////////////
// TYPEDEFS //
//////////////
Texture2D shaderTexture : register(t0);
Texture2D shaderNormalMap : register(t1);
SamplerState SampleType : register(s1);

#define MAX_NUM_LIGHTS 8

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
    const float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];
    float4 normalMap = shaderNormalMap.Sample(SampleType, input.tex);

    normalMap = (normalMap * 2.0f) - 1.0f;

    float3 bumpNormal = (normalMap.x * input.tangent) + (normalMap.y * input.binormal) + (normalMap.z * input.normal);
    bumpNormal = normalize(bumpNormal);

    float3 reflection[MAX_NUM_LIGHTS];
    float4 specular[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.lightPos[i], bumpNormal));
        colorArray[i] = lightColor[i] * lightIntensity[i];
        reflection[i] = normalize(2.0f * lightIntensity[i] * input.normal - input.lightPos[i]);
        specular[i] = pow(saturate(dot(reflection[i], input.lightPos[i])), specularPower);
    }

    float4 colorSum = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 specularSum = float4(0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        specularSum.r += specular[i].r;
        specularSum.g += specular[i].g;
        specularSum.b += specular[i].b;
    }

    float4 color = textureColor * colorSum;

    color = saturate(color + specularSum);

    return color;
}
