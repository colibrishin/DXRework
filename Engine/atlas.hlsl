#include "common.hlsli"
#include "vs_default.hlsl"

float4 SampleAtlas(in uint instance, in float2 texCoord)
{
#define PARAM_ANIM_IDX bufInstance[instance].iParam[0].z
#define PARAM_ATLAS_X bufInstance[instance].iParam[1].x
#define PARAM_ATLAS_Y bufInstance[instance].iParam[1].y
#define PARAM_ATLAS_W bufInstance[instance].iParam[1].z
#define PARAM_ATLAS_H bufInstance[instance].iParam[1].w

  // Change texture coordination to atlas coordination
  const float u = (PARAM_ATLAS_X + texCoord.x * PARAM_ATLAS_W) / PARAM_ATLAS_W;
  const float v = (PARAM_ATLAS_Y + texCoord.y * PARAM_ATLAS_H) / PARAM_ATLAS_H;

  return texAtlases.Sample(PSSampler, float3(PARAM_ATLAS_X, PARAM_ATLAS_Y, PARAM_ANIM_IDX));
#undef PARAM_ANIM_IDX
#undef PARAM_ATLAS_X
#undef PARAM_ATLAS_Y
#undef PARAM_ATLAS_W
#undef PARAM_ATLAS_H
}

float4 ps_main(in PixelInputType input, in uint instanceId : SV_InstanceId) : SV_TARGET
{
  return SampleAtlas(instanceId, input.tex);
}