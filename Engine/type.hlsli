#ifndef __TYPE_HLSLI__
#define MAX_BONE_COUNT 4
#define MAX_NUM_CASCADES 3
#define MAX_NUM_LIGHTS 8
#define MAX_NUM_SLOTS 8
#define MAX_PARAM_TYPE_SLOTS 8

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
  int4 type : LIGHTTYPE;
  float4 range : LIGHTRANGE;
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
  float3 position : POSITION0;
  float4 color : COLOR0;
  float2 tex : TEXCOORD0;

  float3 normal : NORMAL0;
  float3 tangent : TANGENT0;
  float3 binormal : BINOARML0;

  VertexBoneElement bone_element : BONE;
};

struct PixelInputType
{
  float4 position : SV_Position;
  float4 world_position : POSITION0;
  float4 color : COLOR0;
  float2 tex : TEXCOORD0;

  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float3 binormal : BINOARML;

  float4 reflection : POSITION1;
  float4 refraction : POSITION2;

  float3 viewDirection : TEXCOORD2;
  float3 lightDelta[MAX_NUM_LIGHTS] : TEXCOORD3;

  float clipSpacePosZ : SV_ClipDistance0;
  float clipPlane : SV_ClipDistance1;
};

struct InstanceElement
{
  float4  fParam[MAX_PARAM_TYPE_SLOTS];
  int4    iParam[MAX_PARAM_TYPE_SLOTS];
  float4 vParam[MAX_PARAM_TYPE_SLOTS];
  matrix mParam[MAX_PARAM_TYPE_SLOTS];
};

#endif // __TYPE_HLSLI__
