#include "common.hlsli"

RWTexture2D<float4> g_output : register(u0);

RaytracingAccelerationStructure g_tlas : register(t0);

struct Payload
{
  float4 colorAndDist;
};

struct Attributes
{
  float2 barycentrics;
};

[shader("raygeneration")]
void raygen_main()
{
  // Calculate the ray direction in screen space
  uint2 dispatchRaysIndex = DispatchRaysIndex().xy;
  uint2 dispatchRaysDimensions = DispatchRaysDimensions().xy;

  const float2 mid_idx = dispatchRaysIndex.xy + 0.5f;
  float2 ndc = mid_idx / dispatchRaysDimensions.xy * 2.f - 1.f;
  ndc.y = -ndc.y;

  float4 world = mul(float4(ndc, 0, 1), g_camInvVP);
  world /= world.w;

  float4 origin = mul(float4(0, 0, 0, 1), g_camWorld);
  origin /= origin.w;

  RayDesc ray;
  ray.Origin = origin.xyz;
  ray.Direction = normalize(world.xyz - ray.Origin.xyz);
  ray.TMin = 0.001f;
  ray.TMax = 1000.0f;

  Payload payload;

  TraceRay(g_tlas, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);

  g_output[dispatchRaysIndex.xy] = payload.colorAndDist;
}


[shader("closesthit")]
void closest_hit_main(inout Payload payload, Attributes attr)
{
  payload.colorAndDist = float4(1.0f, 0.0f, 0.0f, 1.0f);
  payload.colorAndDist.w = RayTCurrent();
}

[shader("miss")]
void miss_main(inout Payload payload)
{
  payload.colorAndDist = float4(g_ambientColor.xyz, 1.0f);
}