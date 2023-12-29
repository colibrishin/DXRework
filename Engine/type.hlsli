#ifndef __TYPE_HLSLI__
#define MAX_BONE_COUNT 4
#define MAX_NUM_CASCADES 3
#define MAX_NUM_LIGHTS 8
#define MAX_NUM_SLOTS 8

struct CascadeShadow
{
    matrix g_shadowView[MAX_NUM_CASCADES] : SHADOWVIEW;
    matrix g_shadowProj[MAX_NUM_CASCADES] : SHADOWPROJ;
    float4 g_shadowZClip[MAX_NUM_CASCADES] : SHADOWZCLIP;
};

struct VertexBoneElement
{
    int boneIndex[MAX_BONE_COUNT] : BONEINDEX;
    float boneWeight[MAX_BONE_COUNT] : BONEWEIGHT;
    uint bone_count : BONECOUNT;
};

struct BoneTransformElement
{
    matrix transform : BONETRANSFORM;
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

struct GeometryShadowInputType
{
    float4 position : SV_POSITION;
};

struct PixelShadowInputType
{
    float4 position : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

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

#endif // __TYPE_HLSLI__