#ifndef __TYPE_HLSLI__
#define MAX_BONE_COUNT 4
#define MAX_NUM_CASCADES 3
#define MAX_NUM_LIGHTS 8
#define MAX_PARAM_TYPE_SLOTS 8

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
	int4   type : LIGHTTYPE;
	float4 range : LIGHTRANGE;
	float4 radius : LIGHTRADIUS;
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
	float4 worldPosition : POSITION0;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;

	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINOARML;

	float4 reflection : POSITION1;
	float4 refraction : POSITION2;
	float3 scale : POSITION3;

	float3 viewDirection : TEXCOORD2;
	float3 lightDelta[MAX_NUM_LIGHTS] : TEXCOORD3;

	float clipSpacePosZ : SV_ClipDistance0;
	float clipPlane : SV_ClipDistance1;
	uint  instanceId : SV_InstanceID;
};

#define INST_ANIM_FRAME(INSTANCE)    bufInstance[INSTANCE].fParam[0].x
#define INST_SPECULAR(INSTANCE)      bufInstance[INSTANCE].fParam[0].y
#define INST_REFLECT_TRS(INSTANCE)   bufInstance[INSTANCE].fParam[0].z
#define INST_REFLECT_SCL(INSTANCE)   bufInstance[INSTANCE].fParam[0].w
#define INST_REFRACT_SCL(INSTANCE)   bufInstance[INSTANCE].fParam[1].x
#define INST_PARTICLE_LIFE(INSTANCE) uavInstance[INSTANCE].fParam[1].y

#define INST_BONE_FLAG(INSTANCE) bufInstance[INSTANCE].iParam[0].x
#define INST_ANIM_DURATION(INSTANCE) bufInstance[INSTANCE].iParam[0].y
#define INST_ANIM_IDX(INSTANCE)  bufInstance[INSTANCE].iParam[0].z
#define INST_NO_ANIM(INSTANCE)   bufInstance[INSTANCE].iParam[0].w
#define INST_ATLAS_X(INSTANCE) bufInstance[INSTANCE].iParam[1].x
#define INST_ATLAS_Y(INSTANCE) bufInstance[INSTANCE].iParam[1].y
#define INST_ATLAS_W(INSTANCE) bufInstance[INSTANCE].iParam[1].z
#define INST_ATLAS_H(INSTANCE) bufInstance[INSTANCE].iParam[1].w
#define INST_REPEAT_TEX(INSTANCE) bufInstance[INSTANCE].iParam[2].x
#define INST_ATLAS_FLAG(INSTANCE) bufInstance[INSTANCE].iParam[2].y
#define INST_TEX_SLOT_OFFSET(INSTANCE) bufInstance[INSTANCE].iParam[2].z
#define INST_TEX_SLOT0(INSTANCE) bufInstance[INSTANCE].iParam[2].w
#define INST_TEX_SLOT1(INSTANCE) bufInstance[INSTANCE].iParam[3].x
#define INST_TEX_SLOT2(INSTANCE) bufInstance[INSTANCE].iParam[3].y
#define INST_TEX_SLOT3(INSTANCE) bufInstance[INSTANCE].iParam[3].z
#define INST_PARTICLE_ACTIVE(INSTANCE) uavInstance[INSTANCE].iParam[3].w

#define INST_OVERRIDE_COL(INSTANCE) bufInstance[INSTANCE].vParam[0]
#define INST_SPECULAR_COL(INSTANCE) bufInstance[INSTANCE].vParam[1]
#define INST_CLIP_PLANE(INSTANCE) bufInstance[INSTANCE].vParam[2]
#define INST_PARTICLE_VELOCITY(INSTANCE) uavInstance[INSTANCE].vParam[3]

#define INST_WORLD(INSTANCE)     bufInstance[INSTANCE].mParam[0]
#define INST_PARTICLE_WORLD(INSTANCE)  uavInstance[INSTANCE].mParam[0]

struct ParamElement
{
	float4 fParam[MAX_PARAM_TYPE_SLOTS];
	int4   iParam[MAX_PARAM_TYPE_SLOTS];
	float4 vParam[MAX_PARAM_TYPE_SLOTS];
	matrix mParam[MAX_PARAM_TYPE_SLOTS];
};

struct Attributes
{
	float2 barycentrics;
};

#endif // __TYPE_HLSLI__
