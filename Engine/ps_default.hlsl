#include "common.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
	const float4 textureColor = shaderTexture.Sample(PSSampler, input.tex);

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
	    lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));
        colorArray[i] = lightColor[i] * lightIntensity[i];
    }

    float4 colorSum = float4(0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    float4 color = saturate(colorSum) * textureColor;

	return color;
}
