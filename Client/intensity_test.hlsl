#include "common.hlsli"
#include "vs_default.hlsl"

#define EMPTY_DEPTH 1.f

// overriding the default definition of common.hlsli
Texture2DArray maskTex[MAX_NUM_LIGHTS] : register(t2);

struct PixelIntensityOutput
{
  uint4  availability : SV_Target0;
  float4 worldPosition : SV_Target1;
};

float GetMaskFactorImpl(in int lightIndex, in int cascadeIndex, in float4 world_position)
{
  float4 projCoords = world_position / world_position.w;
  projCoords.x      = projCoords.x * 0.5 + 0.5f;
  projCoords.y      = -projCoords.y * 0.5 + 0.5f;

  if (projCoords.z > 1.0) { return 0.f; }

  float currentDepth = projCoords.z;
  float bias         = 0.01f;
  float4 mask        = 0.0f;

  float3 samplePos = float3(projCoords.xyz);
  samplePos.z = cascadeIndex;

  Texture2DArray shadowMap = maskTex[lightIndex];

  [unroll]
  for (int x = -1; x <= 1; ++x)
  {
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
      mask += shadowMap.Sample(PSSampler, samplePos);
    }
  }

  mask /= 9.0f;
  return mask.x;
}

void GetMaskFactor(
  in float4 world_position, in float z_clip,
  out float maskFactor[MAX_NUM_LIGHTS]
)
{
  int i = 0;
  int j = 0;

  for (i = 0; i < MAX_NUM_LIGHTS; ++i)
  {
    maskFactor[i] = 0.0f;
  }

  [unroll]
  for (i = 0; i < MAX_NUM_LIGHTS; ++i)
  {
#define PARAM_LIGHT_COUNT g_iParam[0].x
    if (i > PARAM_LIGHT_COUNT) { break; }
#undef PARAM_LIGHT_COUNT

    [unroll]
    for (j = 0; j < MAX_NUM_CASCADES; ++j)
    {
      const matrix vp = mul
        (
         bufLightVP[i].g_shadowView[j],
         bufLightVP[i].g_shadowProj[j]
        );
      const float4 position = mul(world_position, vp);

      if (z_clip <= bufLightVP[i].g_shadowZClip[j].z)
      {
        maskFactor[i] = GetMaskFactorImpl(i, j, position);
        break;
      }
    }
  }
}

PixelIntensityOutput ps_main(PixelInputType input)
{
  float shadowFactor[MAX_NUM_LIGHTS];
  float maskFactor[MAX_NUM_LIGHTS];
  bool  intersection = false;
  int   availability = 0;

  GetShadowFactor(input.worldPosition, input.clipSpacePosZ, shadowFactor);
  GetMaskFactor(input.worldPosition, input.clipSpacePosZ, maskFactor);

  [unroll]
  for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
  {
    const float dist = length(input.lightDelta[i]);

    const float3 lightDir = normalize(input.lightDelta[i]);
    float intensity = saturate(dot(input.normal, lightDir));
    intensity *= saturate(1.0f - (dist / bufLight[i].range.x));

    if (i > PARAM_NUM_LIGHT) { break; }

    if (bufLight[i].type.x != LIGHT_TYPE_SPOT) { continue; }

    if (dist > bufLight[i].range.x) continue;

    if (intensity > 0.f && shadowFactor[i] != 1.f && maskFactor[i] > 0.f)
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
    output.worldPosition = input.worldPosition;
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
