#include "common.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
    const float4 textureColor = input.color;

    float lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    float3 reflection[MAX_NUM_LIGHTS];
    float4 specular[MAX_NUM_LIGHTS];

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, -input.lightDirection[i]));
        colorArray[i] = lightColor[i] * lightIntensity[i];
        reflection[i] = normalize(2.0f * lightIntensity[i] * input.normal - input.lightDirection[i]);
        specular[i] = pow(saturate(dot(reflection[i], input.viewDirection)), specularPower);
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

    const float color = textureColor * saturate(colorSum) + specularSum;

    return color;
}
