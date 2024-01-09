#include "common.hlsli"

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    int i = 0;

    float shadowFactor[MAX_NUM_LIGHTS];
    GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

    const float4 textureColor = tex00.Sample(PSSampler, input.tex);

    float  lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];

    float3 reflection[MAX_NUM_LIGHTS];
    float4 specular[MAX_NUM_LIGHTS];

    for (i = 0; i < g_lightCount.x; ++i)
    {
        lightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));
        colorArray[i]     =
                LerpShadow(shadowFactor[i]) * bufLight[i].color * lightIntensity[i];
        reflection[i] = normalize(
                                  2.0f * lightIntensity[i] * input.normal -
                                  input.lightDirection[i]);
        specular[i] =
                pow(saturate(dot(reflection[i], input.viewDirection)), g_specularPower);
    }

    float4 colorSum    = g_ambientColor;
    float4 specularSum = g_ambientColor;

    for (i = 0; i < g_lightCount.x; ++i)
    {
        colorSum.r += colorArray[i].r;
        colorSum.g += colorArray[i].g;
        colorSum.b += colorArray[i].b;
    }

    for (i = 0; i < g_lightCount.x; ++i)
    {
        specularSum.r += specular[i].r;
        specularSum.g += specular[i].g;
        specularSum.b += specular[i].b;
    }

    const float4 color = textureColor * saturate(colorSum + specularSum);

    return color;
}
