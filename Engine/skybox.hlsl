#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
    const float4 textureColor = tex00.Sample(PSSampler, input.tex);

    float4 color = textureColor;

    return color;
}
