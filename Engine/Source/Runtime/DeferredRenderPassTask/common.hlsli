#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define TRIANGLE_MACRO 3
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOT 2
#define MAX_TEX_PER_MAT 8
#define FLT_MAX 3.402823466e+38
#define PI 3.14159265359f
#define EPSILON 1e-6f

#include "type.hlsli"
#include "utility.hlsli"

SamplerState           PSSampler : register(s0);
SamplerComparisonState PSShadowSampler : register(s1);

RWTexture1D<float4>              uav00 : register(u0);
RWTexture1D<float4>              uav01 : register(u1);
RWTexture2D<float4>              uav02 : register(u2);
RWTexture2D<float4>              uav03 : register(u3);
RWTexture2DArray<float4>         uavArr00 : register(u4);
RWTexture2DArray<float4>         uavArr01 : register(u5);
RWStructuredBuffer<ParamElement> uavInstance : register(u6);

Texture2D      tex[16]  : register(t0);
Texture2DArray texArr00 : register(t16);
Texture2DArray texArr01 : register(t17);
TextureCube    texCube00 : register(t18);
TextureCube    texCube01 : register(t19);
Texture1D      tex1d00 : register(t20);
Texture1D      tex1d01 : register(t21);

StructuredBuffer<LightElement>         bufLight : register(t22);
StructuredBuffer<CascadeShadowElement> bufLightVP : register(t23);
StructuredBuffer<ParamElement>         bufInstance : register(t24);
StructuredBuffer<ParamElement>         bufLocalParam : register(t25);
Texture2D                              texRendered : register(t26);
Texture2DArray                         texShadowMap[MAX_NUM_LIGHTS] : register(t27);
Texture3D                              texAnimations : register(t35);
Texture3D                              texAtlases : register(t36);

static const float4 g_ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

cbuffer PerspectiveBuffer : register(b0)
{
	matrix g_camWorld;
	matrix g_camView;
	matrix g_camProj;

	matrix g_camInvView;
	matrix g_camInvProj;
	matrix g_camInvVP;

	matrix g_camReflectView;
};

cbuffer ParamBuffer : register(b1)
{
	float4 g_fParam[MAX_PARAM_TYPE_SLOTS];
	int4   g_iParam[MAX_PARAM_TYPE_SLOTS];
	float4 g_vParam[MAX_PARAM_TYPE_SLOTS];
	matrix g_mParam[MAX_PARAM_TYPE_SLOTS];
};

cbuffer ViewportBuffer : register(b2)
{
	float2 g_viewResolution;
}

float4 SampleGrad(in SamplerState inSampler, in float2 uv, in float2 ddx, in float2 ddy, in uint slot)
{
    float4 outValue = float4(0.f, 0.f, 0.f, 0.f);

    if (slot >= MAX_TEX_PER_MAT)
    {
        return outValue;
    }

#define SWITCH_OFFSET(NUM) case NUM: \
    outValue = tex[NUM].SampleGrad(inSampler, uv, ddx, ddy); \
    break;

    switch (slot)
    {
        SWITCH_OFFSET(0)
        SWITCH_OFFSET(1)
        SWITCH_OFFSET(2)
        SWITCH_OFFSET(3)
        SWITCH_OFFSET(4)
        SWITCH_OFFSET(5)
        SWITCH_OFFSET(6)
        SWITCH_OFFSET(7)
        SWITCH_OFFSET(8)
        SWITCH_OFFSET(9)
        SWITCH_OFFSET(10)
        SWITCH_OFFSET(11)
        SWITCH_OFFSET(12)
        SWITCH_OFFSET(13)
        SWITCH_OFFSET(14)
        SWITCH_OFFSET(15)
        default:
            break;
    }
#undef SWITCH_OFFSET

    return outValue;
}

float4 Sample(in SamplerState inSampler, in float2 uv, in uint slot)
{
	float4 outValue = float4(0.f, 0.f, 0.f, 0.f);

	if (slot >= MAX_TEX_PER_MAT)
	{
		return outValue;
	}
	
#define SWITCH_OFFSET(NUM) case NUM: \
    outValue = tex[NUM].Sample(inSampler, uv); \
    break;

    switch (slot)
    {
        SWITCH_OFFSET(0)
        SWITCH_OFFSET(1)
        SWITCH_OFFSET(2)
        SWITCH_OFFSET(3)
        SWITCH_OFFSET(4)
        SWITCH_OFFSET(5)
        SWITCH_OFFSET(6)
        SWITCH_OFFSET(7)
        SWITCH_OFFSET(8)
        SWITCH_OFFSET(9)
        SWITCH_OFFSET(10)
        SWITCH_OFFSET(11)
        SWITCH_OFFSET(12)
        SWITCH_OFFSET(13)
        SWITCH_OFFSET(14)
        SWITCH_OFFSET(15)
        default:
            break;
    }
#undef SWITCH_OFFSET
	
	return outValue;
}

float4 SampleAtlas(in StructuredBuffer<ParamElement> buffer, in uint instance, in float2 texCoord)
{
	// Change texture coordination to atlas coordination
    uint width, height, depth, numMips;
    texAtlases.GetDimensions(0, width, height, depth, numMips);

    const float fullWidth = float(width);
    const float fullHeight = float(height);

    const float uRangeStart = float(INST_ATLAS_X(buffer, instance)) / fullWidth;
    const float uRangeEnd = float(INST_ATLAS_X(buffer, instance) + INST_ATLAS_W(buffer, instance)) / fullWidth;

    const float vRangeStart = float(INST_ATLAS_Y(buffer, instance)) / fullHeight;
    const float vRangeEnd = float(INST_ATLAS_Y(buffer, instance) + INST_ATLAS_H(buffer, instance)) / fullHeight;

    const float u = lerp(uRangeStart, uRangeEnd, texCoord.x);
    const float v = lerp(vRangeStart, vRangeEnd, texCoord.y);

    return texAtlases.Sample(PSSampler, float3(u, v, INST_ANIM_IDX(buffer, instance)));
}


float GetShadowFactorImpl(
	int    lightIndex, int cascadeIndex,
	float4 cascadeLocalPosition
)
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
	// matrix ctor will transpose the matrix.
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

	for (i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		shadowFactor[i] = 1.0f;
	}
	[unroll] for (i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
#define PARAM_LIGHT_COUNT g_iParam[0].x
		if (i >= PARAM_LIGHT_COUNT)
		{
			break;
		}
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

float4 LerpShadow(in float shadowFactor)
{
	return lerp(float4(0, 0, 0, 1), float4(1, 1, 1, 1), shadowFactor);
}

// Christian Schuler, "Normal Mapping without Precomputed Tangents", ShaderX 5, Chapter 2.6, pp. 131-140
// See also follow-up blog post: http://www.thetenthplanet.de/archives/1180
float3x3 CalculateTBN(float3 p, float3 n, float2 tex)
{
    float3 dp1 = ddx(p);
    float3 dp2 = ddy(p);
    float2 duv1 = ddx(tex);
    float2 duv2 = ddy(tex);

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    float3 t = normalize(mul(float2(duv1.x, duv2.x), inverseM));
    float3 b = normalize(mul(float2(duv1.y, duv2.y), inverseM));
    return float3x3(t, b, n);
}

float3 BiasX2(float3 normal)
{
    return 2.0f * normal - 1.0f;
}

float3 PeturbNormal(float3 localNormal, float3 position, float3 normal, float2 texCoord)
{
    const float3x3 TBN = CalculateTBN(position, normal, texCoord);
    return normalize(mul(localNormal, TBN));
}

#endif // __COMMON_HLSLI__
