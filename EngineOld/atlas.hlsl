#include "common.hlsli"
#include "vs_default.hlsl"

float4 SampleAtlas(in uint instance, in float2 texCoord)
{
#define PARAM_ANIM_IDX bufInstance[instance].iParam[0].y
#define PARAM_ATLAS_X bufInstance[instance].iParam[0].w
#define PARAM_ATLAS_Y bufInstance[instance].iParam[1].x
#define PARAM_ATLAS_W bufInstance[instance].iParam[1].y
#define PARAM_ATLAS_H bufInstance[instance].iParam[1].z

	// Change texture coordination to atlas coordination
	uint width, height, depth, numMips;
	texAtlases.GetDimensions(0, width, height, depth, numMips);

	const float fullWidth  = float(width);
	const float fullHeight = float(height);

	const float uRangeStart = float(PARAM_ATLAS_X) / fullWidth;
	const float uRangeEnd   = float(PARAM_ATLAS_X + PARAM_ATLAS_W) / fullWidth;

	const float vRangeStart = float(PARAM_ATLAS_Y) / fullHeight;
	const float vRangeEnd   = float(PARAM_ATLAS_Y + PARAM_ATLAS_H) / fullHeight;

	const float u = lerp(uRangeStart, uRangeEnd, texCoord.x);
	const float v = lerp(vRangeStart, vRangeEnd, texCoord.y);

	return texAtlases.Sample(PSSampler, float3(u, v, PARAM_ANIM_IDX));
#undef PARAM_ANIM_IDX
#undef PARAM_ATLAS_X
#undef PARAM_ATLAS_Y
#undef PARAM_ATLAS_W
#undef PARAM_ATLAS_H
}

float4 ps_main(in PixelInputType input) : SV_TARGET
{
	return SampleAtlas(input.instanceId, input.tex);
}
