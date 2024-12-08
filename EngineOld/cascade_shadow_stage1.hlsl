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

#define INST_ANIM_FRAME  fParam[0].x
#define INST_ANIM_DURATION iParam[0].x
#define INST_ANIM_IDX  iParam[0].y
#define INST_NO_ANIM   iParam[0].z
#define INST_WORLD     mParam[0]

	if (bufMaterial[0].bindFlag.boneFlag.x && !bufInstance[instanceId].INST_NO_ANIM)
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

			animation_transform += transform * weight;
		}

		output.position = mul(output.position, animation_transform);
	}

	const matrix world = bufInstance[instanceId].INST_WORLD;
#undef INST_NO_ANIM
#undef INST_ANIM_IDX
#undef INST_ANIM_FRAME
#undef INST_ANIM_DURATION
#undef INST_WORLD

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
