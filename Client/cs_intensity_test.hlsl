#include "common.hlsli"

struct LightTable
{
  uint4 table[MAX_NUM_LIGHTS][MAX_NUM_LIGHTS];
};

RWStructuredBuffer<LightTable> g_lightTable : register(u7);

#define TEXTURE_ARRAY uavArr00
#define PARAM_NUM_LIGHTS g_iParam[0].x

[maxthreads(32, 32, 1)]
void cs_main(uint3 tId : SV_DispatchThreadID)
{
#define PARAM_TARGET_LIGHT g_iParam[0].y
  // designated pixel (512 x 512, if shadow map size is not modified)

  const uint group_size = 32 * 8;
  const uint flat_idx = tId.x + tId.y * group_size;

  // ~250,000 pixels
  const uint width  = 512;
  const uint height = 512;

  if (flat_idx >= width * height) { return; }

  const uint4 texel = TEXTURE_ARRAY.Load
  (
    int4(PARAM_TARGET_LIGHT, flat_idx / width, flat_idx % height, 0)
  );

  for (int j = 0; j < PARAM_NUM_LIGHTS; ++j)
  {
    if (texel.x & (1 << j))
    {
      g_lightTable[0].table[PARAM_TARGET_LIGHT][j].x = 1;
    }
  }

  // todo: uav light table
  // if pixel is non-zero, mark as available
  // e.g,
  // light 1 -> 2, 3
}