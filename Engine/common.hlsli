#ifndef __COMMON_HLSL_
#define MAX_NUM_LIGHTS 8
#define TRIANGLE_MACRO 3
#define MAX_NUM_CASCADES 3
#define MAX_BONE_COUNT 4

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

struct BoneTransformElement
{
    matrix transform;
};

Texture2D                              shaderTexture : register(t0);
Texture2D                              shaderNormalMap : register(t1);
Texture2DArray                         cascadeShadowMap[MAX_NUM_LIGHTS] : register(t2);
Texture2D                              renderedTexture : register(t3);
StructuredBuffer<BoneTransformElement> boneTransformBuffer : register(t4);

static const float4 g_ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

cbuffer PerspectiveBuffer : register(b0)
{
    matrix g_cam_world;
    matrix g_cam_view;
    matrix g_cam_projection;
    matrix g_cam_invView;
    matrix g_cam_invProjection;
    matrix g_cam_reflectView;
};

cbuffer TransformBuffer : register(b1)
{
    matrix g_scale;
    matrix g_rotation;
    matrix g_translation;
};

cbuffer LightBuffer : register(b2)
{
    matrix g_lightWorld[MAX_NUM_LIGHTS];
    float4 g_lightColor[MAX_NUM_LIGHTS];
    int    g_lightCount;
    float3 ___p0;
}

cbuffer SpecularBuffer : register(b3)
{
    float  g_specularPower;
    float3 ___p1;
    float4 g_specularColor;
}

struct CascadeShadow
{
    matrix g_shadow_view[MAX_NUM_CASCADES];
    matrix g_shadow_proj[MAX_NUM_CASCADES];
    float4 g_shadow_z_clip[MAX_NUM_CASCADES];
};

// current light view and projection matrix of each cascade
cbuffer CascadeShadowBuffer : register(b4)
{
    CascadeShadow g_currentShadow;
}

cbuffer CascadeShadowChunk : register(b5)
{
    CascadeShadow g_cascadeShadowChunk[MAX_NUM_LIGHTS];
}

cbuffer WaterBuffer : register(b6)
{
    float  g_waterTranslation;
    float  g_reflfrScale;
    float2 ___p2;
}

cbuffer ClipPlaneBuffer : register(b7)
{
    float4 g_clip_plane;
}

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

struct GeometryShadowInputType
{
    float4 position : SV_POSITION;
};

struct PixelShadowStage1InputType
{
    float4 position : SV_POSITION;
    uint   RTIndex : SV_RenderTargetArrayIndex;
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

float4 GetWorldPosition(in matrix mat)
{
    return float4(mat._14, mat._24, mat._34, mat._44);
}

float GetShadowFactorImpl(
    int    lightIndex, int cascadeIndex,
    float4 cascadeLocalPosition)
{
    float4 projCoords = cascadeLocalPosition / cascadeLocalPosition.w;
    projCoords.x      = projCoords.x * 0.5 + 0.5f;
    projCoords.y      = -projCoords.y * 0.5 + 0.5f;

    if (projCoords.z > 1.0)
    {
        return 0.f;
    }

    float currentDepth = projCoords.z;
    float bias         = 0.01f;
    float shadow       = 0.0;

    float3 samplePos = float3(projCoords.xyz);
    samplePos.z      = cascadeIndex;

    Texture2DArray shadowMap = cascadeShadowMap[lightIndex];

    [unroll] for (int x = -1; x <= 1; ++x)
    {
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            shadow += shadowMap.SampleCmpLevelZero(
                                                   PSShadowSampler, samplePos,
                                                   currentDepth - bias, int2(x, y));
        }
    }

    shadow /= 9.0f;
    return shadow;
}

void GetShadowFactor(
    in float4 world_position, in float z_clip,
    out float shadowFactor[MAX_NUM_LIGHTS])
{
    int i = 0;
    int j = 0;

    for (i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        shadowFactor[i] = 1.0f;
    }

    [unroll] for (i = 0; i < g_lightCount; ++i)
    {
        [unroll] for (j = 0; j < MAX_NUM_CASCADES; ++j)
        {
            const matrix vp = mul(
                                  g_cascadeShadowChunk[i].g_shadow_view[j],
                                  g_cascadeShadowChunk[i].g_shadow_proj[j]);
            const float4 position = mul(world_position, vp);

            if (z_clip <= g_cascadeShadowChunk[i].g_shadow_z_clip[j].z)
            {
                shadowFactor[i] = GetShadowFactorImpl(i, j, position);
                break;
            }
        }
    }
}

float4 LerpShadow(in float4 shadowFactor)
{
    return lerp(float4(0, 0, 0, 1), float4(1, 1, 1, 1), shadowFactor);
}

#endif
