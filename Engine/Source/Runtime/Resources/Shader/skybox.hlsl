#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
    const float4 textureColor = Sample(PSSampler, input.tex, INST_TEX_SLOT0(input.instanceId));

	float4 color = textureColor;

	return color;
}
