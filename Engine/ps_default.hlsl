//////////////
// TYPEDEFS //
//////////////
Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s1);

cbuffer LightBuffer : register(b2)
{
    float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
	float _p0;
}

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
    float4 color;
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);

    color = ambientColor;
	
	const float3 lightDir = -lightDirection;
	const float lightIntensity = saturate(dot(input.normal, lightDir));

    if (lightIntensity > 0.0f)
    {
        color += (diffuseColor * lightIntensity);
    }

    color = saturate(color);

    color *= textureColor;

	return color;
}
