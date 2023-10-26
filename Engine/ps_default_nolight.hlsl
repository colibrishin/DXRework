//////////////
// TYPEDEFS //
//////////////
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);

#define MAX_NUM_LIGHTS 8

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

    float4 color = textureColor;

    return color;
}
