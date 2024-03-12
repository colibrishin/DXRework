#include "common.hlsli"
#include "vs_default.hlsl"

struct PixelIntensityOutput
{
  uint4 availability : SV_Target0;
  float4 worldPosition : SV_Target1;
};

PixelIntensityOutput ps_main(PixelInputType input)
{
  float shadowFactor[MAX_NUM_LIGHTS];
  bool intersection = false;
  int availability = 0;

  GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

#define PARAM_TARGET_LIGHT g_iParam[0].z
  shadowFactor[PARAM_TARGET_LIGHT] = 0.f;

  const float dist = length(input.lightDelta[PARAM_TARGET_LIGHT]);

  const float3 lightDir  = normalize(input.lightDelta[PARAM_TARGET_LIGHT]);
  float        intensity = saturate(dot(input.normal, lightDir));
  intensity *= saturate(1.0f - (dist / bufLight[PARAM_TARGET_LIGHT].range.x));

  for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    if (bufLight[i].type.x != LIGHT_TYPE_SPOT)
    {
        continue;
    }

    if (i == PARAM_TARGET_LIGHT) continue;
#undef PARAM_TARGET_LIGHT
    if (dist > bufLight[i].range.x) continue;

    if (intensity > 0.f && shadowFactor[i] != 1.f)
    {
      availability |= 1 << i;
      intersection = true;
    }
  }

  // todo: expandable bit-masking or alternative for more lights
  if (intersection)
  {
    PixelIntensityOutput output;
    output.availability  = uint4(availability, 0, 0, 1);
    output.worldPosition = input.world_position;
    return output;
  }
  else
  {
    PixelIntensityOutput output;
    output.availability  = uint4(0, 0, 0, 0);
    output.worldPosition = float4(0.f, 0.f, 0.f, 0.f);
    return output;
  }
}
