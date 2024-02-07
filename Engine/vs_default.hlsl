#include "common.hlsli"

#define PARAM_NUM_LIGHT g_iParam[0].x

struct PixelInputType
{
  float4 position : SV_POSITION;
  float4 world_position : POSITION0;
  float4 color : COLOR;
  float2 tex : TEXCOORD0;

  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float3 binormal : BINOARML;

  float4 reflection : POSITION1;
  float4 refraction : POSITION2;

  float3 viewDirection : TEXCOORD2;
  float3 lightDirection[MAX_NUM_LIGHTS] : TEXCOORD3;

  float clipSpacePosZ : SV_ClipDistance0;
  float clipPlane : SV_ClipDistance1;
};

PixelInputType vs_main(VertexInputType input, uint instanceId : SV_InstanceID)
{
  PixelInputType output;

  output.position = float4(input.position, 1.0f);

  output.normal   = input.normal;
  output.tangent  = input.tangent;
  output.binormal = input.binormal;

#define INST_ANIM_FRAME  fParam[0].x
#define INST_ANIM_DURATION iParam[0].x   
#define INST_ANIM_IDX  iParam[0].y
#define INST_NO_ANIM   iParam[0].z
#define INST_WORLD     mParam[0]

  if (g_bindFlag.boneFlag.x && !bufInstance[instanceId].INST_NO_ANIM)
  {
    matrix animation_transform;

    for (int i = 0; i < input.bone_element.bone_count; ++i)
    {
      const int    bone_index = input.bone_element.boneIndex[i];
      const float  weight     = input.bone_element.boneWeight[i];
      const matrix transform  = LoadAnimation
        (
         bufInstance[instanceId].INST_ANIM_IDX,
         bufInstance[instanceId].INST_ANIM_FRAME,
         bufInstance[instanceId].INST_ANIM_DURATION,
         bone_index
        );

      animation_transform = (transform * weight) + animation_transform;
    }

    output.position = mul(output.position, animation_transform);

    output.normal   = mul(input.normal, (float3x3)animation_transform);
    output.tangent  = mul(input.tangent, (float3x3)animation_transform);
    output.binormal = mul(input.binormal, (float3x3)animation_transform);
  }

  const matrix world = bufInstance[instanceId].INST_WORLD;

#undef INST_NO_ANIM
#undef INST_ANIM_IDX
#undef INST_ANIM_FPS
#undef INST_ANIM_FRAME
#undef INST_ANIM_DURATION
#undef INST_WORLD

  // Calculate the position of the vertex against the world, view, and
  // projection matrices.
  output.position       = mul(output.position, world);
  output.world_position = output.position;

  output.position = mul(output.position, g_camView);
  output.position = mul(output.position, g_camProj);

  // Store the input color for the pixel shader to use.
  output.color = input.color;
  output.tex   = input.tex;

  [unroll] for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    const float4 light_position = GetTranslation(bufLight[i].world);
    output.lightDirection[i]    = light_position.xyz - output.world_position.xyz;
    output.lightDirection[i]    = normalize(output.lightDirection[i]);
  }

  const float3 cam_position = GetTranslation(g_camWorld);

  output.viewDirection = cam_position.xyz - output.world_position.xyz;
  output.viewDirection = normalize(output.viewDirection);

  output.normal   = mul(output.normal, (float3x3)world);
  output.tangent  = mul(output.tangent, (float3x3)world);
  output.binormal = mul(output.binormal, (float3x3)world);

  matrix reflectionWorld = mul(g_camReflectView, g_camProj);
  reflectionWorld        = mul(world, reflectionWorld);
  output.reflection      = mul(output.position, reflectionWorld);

  matrix vpw        = mul(g_camView, g_camProj);
  vpw               = mul(world, vpw);
  output.refraction = mul(output.position, vpw);

  output.clipSpacePosZ = output.position.z;
  output.clipPlane     = dot(mul(input.position, world), g_clipPlane);

  return output;
}