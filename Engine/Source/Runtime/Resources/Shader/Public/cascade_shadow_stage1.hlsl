#include "common.hlsli"

struct PixelShadowInputType
{
	float4 position : SV_Position;
	uint   RTIndex : SV_RenderTargetArrayIndex;
};

struct GeometryShadowInputType
{
	float4 position : SV_Position;
	uint   instanceId : SV_InstanceID;
};

GeometryShadowInputType vs_main(VertexInputType input, uint instanceId : SV_InstanceID)
{
	GeometryShadowInputType output;

	output.position = float4(input.position, 1.0f);

    if (INST_BONE_FLAG(instanceId) && !INST_NO_ANIM(instanceId))
	{
		matrix animation_transform;

		for (int i = 0; i < input.bone_element.bone_count; ++i)
		{
			const int    bone_index = input.bone_element.boneIndex[i];
			const float  weight     = input.bone_element.boneWeight[i];
			const matrix transform  = LoadAnimation
					(
					 INST_ANIM_IDX(instanceId),
					 INST_ANIM_FRAME(instanceId),
					 INST_ANIM_DURATION(instanceId),
					 bone_index
					);

			animation_transform += transform * weight;
		}

		output.position = mul(output.position, animation_transform);
	}

    const matrix world = INST_WORLD(instanceId);

	output.position   = mul(output.position, world);
	output.instanceId = instanceId;

	return output;
}

[maxvertexcount(TRIANGLE_MACRO * MAX_NUM_CASCADES)]
void gs_main(
	triangle GeometryShadowInputType           input[3],
	inout TriangleStream<PixelShadowInputType> output
)
{
	for (int i = 0; i < MAX_NUM_CASCADES; ++i)
	{
		PixelShadowInputType element;
		element.RTIndex = i;

		for (int j = 0; j < TRIANGLE_MACRO; ++j)
		{
#define TARGET_SHADOW_PARAM bufLocalParam[0].iParam[0].x
			element.position =
					mul
					(
					 input[j].position, mul
					 (
					  bufLightVP[TARGET_SHADOW_PARAM].g_shadowView[i],
					  bufLightVP[TARGET_SHADOW_PARAM].g_shadowProj[i]
					 )
					);
#undef TARGET_SHADOW_PARAM
			output.Append(element);
		}

		output.RestartStrip();
	}
}


float4 ps_main(PixelShadowInputType input) : SV_Target
{
	// Returns 1.0f if the pixel is in the shadow. (for masking)
	return float4(1.f, 1.f, 1.f, 1.f);
}
