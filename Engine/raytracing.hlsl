#include "common.hlsli"

RWTexture2D<float4> g_output : register(u0);

struct Payload
{
  float4 colorAndDist;
};

[shader("raygeneration")]
void raygen_main()
{
  // Calculate the ray direction in screen space
  uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
  uint2 dispatchRaysDimensions = DispatchRaysDimensions().xy;

  g_output[dispatchRaysIndex.xy] = float4(0.f, 0.f, 0.f, 1.0f);
}
