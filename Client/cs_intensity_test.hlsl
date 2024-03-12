#include "common.hlsli"

struct LightTable
{
  uint4 table[MAX_NUM_LIGHTS];
  float2 min[MAX_NUM_LIGHTS];
  float2 max[MAX_NUM_LIGHTS];
};

Texture2D<uint> integerTex00 : register(t0);
RWStructuredBuffer<LightTable> g_lightTable : register(u7);

#define PARAM_NUM_LIGHTS g_iParam[0].x

[numthreads(32, 32, 1)]
void cs_main(uint3 tId : SV_DispatchThreadID)
{
#define PARAM_TARGET_LIGHT g_iParam[0].y
  // designated pixel (512 x 512, if shadow map size is not modified)

  // ~250,000 pixels
  const uint width  = 512;
  const uint height = 512;

  const uint flat_idx   = tId.x * tId.y;

  if (flat_idx >= width * height) { return; }

  const float2 texel_pos = float2
    (
     (flat_idx / width),
     (flat_idx % height)
    );

  const uint4 texel = integerTex00.Load
    (
     int3(texel_pos, 0)
    );

  for (int j = 0; j < PARAM_NUM_LIGHTS; ++j)
  {
    if (texel.x & (1 << j))
    {
      g_lightTable[PARAM_TARGET_LIGHT].table[j].x = 1;

      if (length(texel_pos) < length(g_lightTable[PARAM_TARGET_LIGHT].min[j]))
      {
        g_lightTable[PARAM_TARGET_LIGHT].min[j] = texel_pos;
      }

      if (length(texel_pos) > length(g_lightTable[PARAM_TARGET_LIGHT].max[j]))
      {
        g_lightTable[PARAM_TARGET_LIGHT].max[j] = texel_pos;
      }
    }
  }

  // todo: uav light table
  // if pixel is non-zero, mark as available
  // e.g,
  // light 1 -> 2, 3
}