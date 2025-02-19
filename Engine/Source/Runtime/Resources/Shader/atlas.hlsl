#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(in PixelInputType input) : SV_TARGET
{
    return SampleAtlas(bufInstance, input.instanceId, input.tex);
}
