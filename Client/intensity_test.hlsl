#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
  float shadowFactor[MAX_NUM_LIGHTS];
  int availability = 0;

  GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

#define PARAM_TARGET_LIGHT g_iParam[0].z
  shadowFactor[PARAM_TARGET_LIGHT] = 0.f;

  for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    const float dist = length(input.lightDelta[i]);

    if (bufLight[i].type.x != LIGHT_TYPE_SPOT)
    {
        continue;
    }

    if (i == PARAM_TARGET_LIGHT) continue;
#undef PARAM_TARGET_LIGHT
    if (dist > bufLight[i].range.x) continue;

    const float lightDir = normalize(input.lightDelta[i]);
    float intensity = saturate(dot(input.normal, lightDir));
    intensity *= saturate(1.0f - dist / bufLight[i].range.x);

    if (intensity > 0.f && shadowFactor[i] > 0.f)
    {
      availability |= 1 << i;
    }
  }

  // todo: expandable bit-masking or alternative for more lights
  return float4(availability, 0.f, 0.f, 1.f);
}