#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
    float4 color = float4(0.f, 0.f, 0.f, 0.f);
    if (INST_TEX_SLOT0_ENABLE(bufInstance, input.instanceId) == true)
    {
        color = Sample(PSSampler, input.tex, INST_TEX_SLOT0(bufInstance, input.instanceId));
    }
	
	return color;
}
