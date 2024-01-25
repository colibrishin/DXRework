#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define TRIANGLE_MACRO 3

#include "type.hlsli"
#include "utility.hlsli"

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

Texture2D      tex00 : register(t0);
Texture2D      tex01 : register(t1);
Texture2D      tex02 : register(t2);
Texture2D      tex03 : register(t3);
Texture2D      tex04 : register(t4);
Texture2D      tex05 : register(t5);
Texture2D      tex06 : register(t6);
Texture2D      tex07 : register(t7);
Texture2DArray texArr00 : register(t8);
Texture2DArray texArr01 : register(t9);
Texture2DArray texArr02 : register(t10);
Texture2DArray texArr03 : register(t11);
Texture2DArray texArr04 : register(t12);
Texture2DArray texArr05 : register(t13);
Texture2DArray texArr06 : register(t14);
Texture2DArray texArr07 : register(t15);
TextureCube    texCube00 : register(t16);
TextureCube    texCube01 : register(t17);
TextureCube    texCube02 : register(t18);
TextureCube    texCube03 : register(t19);
TextureCube    texCube04 : register(t20);
TextureCube    texCube05 : register(t21);
TextureCube    texCube06 : register(t22);
TextureCube    texCube07 : register(t23);

Texture2DArray texShadowMap[MAX_NUM_LIGHTS] : register(t32);
Texture2D      texRendered : register(t33);
Texture3D      texAnimations : register(t34);

StructuredBuffer<LightElement>         bufLight : register(t64);
StructuredBuffer<CascadeShadowElement> bufLightVP : register(t65);
StructuredBuffer<InstanceElement>      bufInstance : register(t66);

static const float4 g_ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

cbuffer PerspectiveBuffer : register(b0)
{
  matrix g_camWorld;
  matrix g_camView;
  matrix g_camProj;

  matrix g_camInvView;
  matrix g_camInvProj;

  matrix g_camReflectView;
};

cbuffer TransformBuffer : register(b1)
{
  matrix g_world;
};

cbuffer MaterialBuffer : register(b2)
{
  BindFlag g_bindFlag : BINDFLAG;

  float g_specularPower : SPECULARPOWER;
  float g_reflectionTranslation : REFTRANSLATION;
  float g_reflectionScale : REFSCALE;
  float g_refractionScale : REFRACTSCALE;

  float4 g_overrideColor : OVERRIDECOLOR;
  float4 g_specularColor : SPECULARCOLOR;
  float4 g_clipPlane : CLIPPLANE;
}

cbuffer ParamBuffer : register(b3)
{
  float4 g_fParam[4] : FPARAM;
  int4   g_iParam[4] : IPARAM;
  float4 g_vecParam[4] : VECPARAM;
  matrix g_matParam[4] : MATPARAM;
}

float GetShadowFactorImpl(
  int    lightIndex, int cascadeIndex,
  float4 cascadeLocalPosition
)
{
  float4 projCoords = cascadeLocalPosition / cascadeLocalPosition.w;
  projCoords.x      = projCoords.x * 0.5 + 0.5f;
  projCoords.y      = -projCoords.y * 0.5 + 0.5f;

  if (projCoords.z > 1.0) { return 0.f; }

  float currentDepth = projCoords.z;
  float bias         = 0.01f;
  float shadow       = 0.0;

  float3 samplePos = float3(projCoords.xyz);
  samplePos.z      = cascadeIndex;

  Texture2DArray shadowMap = texShadowMap[lightIndex];

  [unroll] for (int x = -1; x <= 1; ++x)
  {
    [unroll] for (int y = -1; y <= 1; ++y)
    {
      shadow += shadowMap.SampleCmpLevelZero
        (
         PSShadowSampler, samplePos,
         currentDepth - bias, int2(x, y)
        );
    }
  }

  shadow /= 9.0f;
  return shadow;
}

matrix LoadAnimation(in uint anim_idx, in float frame, in uint bone_idx)
{
  const float sampling_rate_frame = (frame * 10);
  const int   idx                 = (int)sampling_rate_frame;

  const int frame_idx      = idx;
  const int next_frame_idx = idx + 1;
  const float t = sampling_rate_frame - frame_idx;

  // todo: check duration
  // if (next_frame_idx >= duration)....

  // since we are storing float4s, bone idx should be
  // multiplied by 4 to get the correct index
  uint u0 = bone_idx * 4;
  uint v0 = frame_idx;
  uint w0 = anim_idx;

  uint u1 = bone_idx * 4;
  uint v1 = next_frame_idx;
  uint w1 = anim_idx;

  float4       r00  = texAnimations.Load(uint4(u0, v0, w0, 0));
  float4       r01  = texAnimations.Load(uint4(u0 + 1, v0, w0, 0));
  float4       r02  = texAnimations.Load(uint4(u0 + 2, v0, w0, 0));
  float4       r03  = texAnimations.Load(uint4(u0 + 3, v0, w0, 0));
  const matrix mat0 = matrix(r00, r01, r02, r03);

  float4       r10  = texAnimations.Load(uint4(u1, v1, w1, 0));
  float4       r11  = texAnimations.Load(uint4(u1 + 1, v1, w1, 0));
  float4       r12  = texAnimations.Load(uint4(u1 + 2, v1, w1, 0));
  float4       r13  = texAnimations.Load(uint4(u1 + 3, v1, w1, 0));
  const matrix mat1 = matrix(r10, r11, r12, r13);

  return lerp(mat0, mat1, t);
}

void GetShadowFactor(
  in float4 world_position, in float z_clip,
  out float shadowFactor[MAX_NUM_LIGHTS]
)
{
  int i = 0;
  int j = 0;

  for (i = 0; i < MAX_NUM_LIGHTS; ++i) { shadowFactor[i] = 1.0f; }

  [unroll] for (i = 0; i < MAX_NUM_LIGHTS; ++i)
  {
    // Assuming light count is bound at idx 0
    if (i > g_iParam[0].x) { break; }

    [unroll] for (j = 0; j < MAX_NUM_CASCADES; ++j)
    {
      const matrix vp = mul
        (
         bufLightVP[i].g_shadowView[j],
         bufLightVP[i].g_shadowProj[j]
        );
      const float4 position = mul(world_position, vp);

      if (z_clip <= bufLightVP[i].g_shadowZClip[j].z)
      {
        shadowFactor[i] = GetShadowFactorImpl(i, j, position);
        break;
      }
    }
  }
}

float4 LerpShadow(in float4 shadowFactor) { return lerp(float4(0, 0, 0, 1), float4(1, 1, 1, 1), shadowFactor); }

#endif // __COMMON_HLSLI__
