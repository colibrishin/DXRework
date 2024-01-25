#ifndef __TYPE_HLSLI__
#define MAX_BONE_COUNT 4
#define MAX_NUM_CASCADES 3
#define MAX_NUM_LIGHTS 8
#define MAX_NUM_SLOTS 8

struct BindFlag
{
  int4 texFlag[MAX_NUM_SLOTS];
  int4 texArrayFlag[MAX_NUM_SLOTS];
  int4 texCubeFlag[MAX_NUM_SLOTS];
  int4 boneFlag;
};

struct CascadeShadowElement
{
  matrix g_shadowView[MAX_NUM_CASCADES] : SHADOWVIEW;
  matrix g_shadowProj[MAX_NUM_CASCADES] : SHADOWPROJ;
  float4 g_shadowZClip[MAX_NUM_CASCADES] : SHADOWZCLIP;
};

struct LightElement
{
  matrix world : LIGHTWORLD;
  float4 color : LIGHTCOLOR;
};

struct BoneTransformElement
{
  matrix transform : BONETRANSFORM;
};

struct VertexBoneElement
{
  int   boneIndex[MAX_BONE_COUNT] : BONEINDEX;
  float boneWeight[MAX_BONE_COUNT] : BONEWEIGHT;
  uint  bone_count : BONECOUNT;
};

struct VertexInputType
{
  float3 position : POSITION;
  float4 color : COLOR;
  float2 tex : TEXCOORD0;

  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float3 binormal : BINOARML;

  VertexBoneElement bone_element : BONE;
};

struct InstanceElement
{
  matrix world : WORLD;
  float4 animFrame : ANIMFRAME;
  int4   boneAnimDuration : ANIMDURATION;
  int4   animIndex : ANIMINDEX;
  int4   noAnimFlag : NOANIMFLAG;
};

struct ParticleElement
{
  float4 position : POSITION;
  float4 size : SIZE;
  float4 color : COLOR;
};

#endif // __TYPE_HLSLI__
