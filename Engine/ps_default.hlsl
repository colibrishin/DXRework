//////////////
// TYPEDEFS //
//////////////
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 tex : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);

    return textureColor;
}