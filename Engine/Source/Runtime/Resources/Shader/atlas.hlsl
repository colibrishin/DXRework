#include "common.hlsli"
#include "vs_default.hlsl"

float4 SampleAtlas(in uint instance, in float2 texCoord)
{
	// Change texture coordination to atlas coordination
	uint width, height, depth, numMips;
	texAtlases.GetDimensions(0, width, height, depth, numMips);

	const float fullWidth  = float(width);
	const float fullHeight = float(height);

	const float uRangeStart = float(INST_ATLAS_X(instance)) / fullWidth;
    const float uRangeEnd = float(INST_ATLAS_X(instance) + INST_ATLAS_W(instance)) / fullWidth;

    const float vRangeStart = float(INST_ATLAS_Y(instance)) / fullHeight;
    const float vRangeEnd = float(INST_ATLAS_Y(instance) + INST_ATLAS_H(instance)) / fullHeight;

	const float u = lerp(uRangeStart, uRangeEnd, texCoord.x);
	const float v = lerp(vRangeStart, vRangeEnd, texCoord.y);

	return texAtlases.Sample(PSSampler, float3(u, v, INST_ANIM_IDX(instance)));
}

float4 ps_main(in PixelInputType input) : SV_TARGET
{
	return SampleAtlas(input.instanceId, input.tex);
}
