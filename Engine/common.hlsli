#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define TRIANGLE_MACRO 3

#include "type.hlsli"
#include "utility.hlsli"

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

RWTexture1D<float4>      uav00 : register(u0);
RWTexture1D<float4>      uav01 : register(u1);
RWTexture2D<float4>      uav02 : register(u2);
RWTexture2D<float4>      uav03 : register(u3);
RWTexture2DArray<float4> uavArr04 : register(u4);
RWTexture2DArray<float4> uavArr05 : register(u5);
RWStructuredBuffer<InstanceElement> uavInstance : register(u6);

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
Texture1D      tex1d00 : register(t24);
Texture1D      tex1d01 : register(t25);
Texture1D      tex1d02 : register(t26);
Texture1D      tex1d03 : register(t27);
Texture1D      tex1d04 : register(t28);
Texture1D      tex1d05 : register(t29);
Texture1D      tex1d06 : register(t30);
Texture1D      tex1d07 : register(t31);
Texture2DArray texShadowMap[MAX_NUM_LIGHTS] : register(t32);
Texture2D      texRendered : register(t33);
Texture3D      texAnimations : register(t34);

StructuredBuffer<LightElement>         bufLight : register(t48);
StructuredBuffer<CascadeShadowElement> bufLightVP : register(t49);
StructuredBuffer<InstanceElement>      bufInstance : register(t50);

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
  float4 g_fParam[MAX_PARAM_TYPE_SLOTS] : FPARAM;
  int4   g_iParam[MAX_PARAM_TYPE_SLOTS] : IPARAM;
  float4 g_vParam[MAX_PARAM_TYPE_SLOTS] : VPARAM;
  matrix g_mParam[MAX_PARAM_TYPE_SLOTS] : MPARAM;
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

matrix GetAnimationMatrix(in uint anim_idx, in uint frame, in uint bone_idx)
{
  // since we are storing float4s, bone idx should be
  // multiplied by 4 to get the correct index

  float4 r0 = texAnimations.Load(uint4(bone_idx * 4, frame, anim_idx, 0));
  float4 r1 = texAnimations.Load(uint4(bone_idx * 4 + 1, frame, anim_idx, 0));
  float4 r2 = texAnimations.Load(uint4(bone_idx * 4 + 2, frame, anim_idx, 0));
  float4 r3 = texAnimations.Load(uint4(bone_idx * 4 + 3, frame, anim_idx, 0));

  return matrix(r0, r1, r2, r3);
}

matrix LoadAnimation(in uint anim_idx, in float frame, in int duration, in uint bone_idx)
{
  const int   frame_idx      = floor(frame);
  const int   next_frame_idx = ceil(frame);
  const float t              = frac(frame);

  matrix matT0, matT1;

  if (frame_idx < 0)
  {
    if (duration == 0.f)
    {
      matT0 = GetAnimationMatrix(anim_idx, 0, bone_idx);
      matT1 = GetAnimationMatrix(anim_idx, 0, bone_idx);
    }
    else
    {
      matT0 = GetAnimationMatrix(anim_idx, 0, bone_idx);
      matT1 = GetAnimationMatrix(anim_idx, 1, bone_idx);
    }
  }
  else if (next_frame_idx > duration || frame_idx > duration)
  {
    matT0 = GetAnimationMatrix(anim_idx, fmod(duration - 2, duration), bone_idx);
    matT1 = GetAnimationMatrix(anim_idx, fmod(duration - 1, duration), bone_idx);
  }
  else
  {
    matT0 = GetAnimationMatrix(anim_idx, frame_idx, bone_idx);
    matT1 = GetAnimationMatrix(anim_idx, next_frame_idx, bone_idx);
  }

  float3     tr0, tr1;
  float3     sc0, sc1;
  quaternion qt0, qt1;

  Decompose(matT0, tr0, sc0, qt0);
  Decompose(matT1, tr1, sc1, qt1);

  const float3     tr_final = lerp(tr0, tr1, t);
  const float3     sc_final = lerp(sc0, sc1, t);
  const quaternion qt_final = SLerp(qt0, qt1, t);

  return Compose(tr_final, sc_final, qt_final);
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
#define PARAM_LIGHT_COUNT g_iParam[0].x
    if (i > PARAM_LIGHT_COUNT) { break; }
#undef PARAM_LIGHT_COUNT

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
