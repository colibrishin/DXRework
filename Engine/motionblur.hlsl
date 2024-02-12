#include "common.hlsli"
#include "vs_default.hlsl"
// From NVIDIA GPU Gems 3, chapter 27.

float4 ps_main(PixelInputType input) : SV_TARGET
{
  // Get the depth buffer value at this pixel.
  float4 depth  = texVelocity.Sample(PSSampler, input.tex);
  float  zOverW = depth.z / depth.w;
  // H is the viewport position at this pixel in the range -1 to 1.
  float4 H = float4(input.tex.x * 2 - 1, (1 - input.tex.y) * 2 - 1, zOverW, 1);

  // Transform by the view-projection inverse.
  const matrix invVP = mul(g_camInvView, g_camInvProj);
  float4       D     = mul(H, invVP);

  // Divide by w to get the world position.
  const float4 worldPos = D / D.w;

  // Current viewport position
  const float4 currentPos = H;

  // Use the world position, and transform by the previous view-projection matrix.
  const matrix prevVP      = mul(g_camPrevView, g_camPrevProj);
  float4       previousPos = mul(worldPos, prevVP);

  // Convert to nonhomogeneous points [-1,1] by dividing by w.
  previousPos /= previousPos.w;

  // Use this frame's position and last frame's to compute the pixel velocity.
  float2 velocity = (currentPos - previousPos) / 2.f;
  velocity.y = -velocity.y;

  return float4(velocity, 0.f, 1.f);
}
