#include "common.hlsli"

PixelInputType vs_main(VertexInputType input, uint instanceId : SV_InstanceID)
{
	PixelInputType output;

	output.position = float4(input.position, 1.0f);

	output.normal   = input.normal;
	output.tangent  = input.tangent;
	output.binormal = input.binormal;

    if (INST_BONE_FLAG(bufInstance, instanceId) && !INST_NO_ANIM(bufInstance, instanceId))
	{
		matrix animation_transform;

		for (int i = 0; i < input.bone_element.bone_count; ++i)
		{
			const int    bone_index = input.bone_element.boneIndex[i];
			const float  weight     = input.bone_element.boneWeight[i];
			const matrix transform  = LoadAnimation
					(
					 INST_ANIM_IDX(bufInstance, instanceId),
					 INST_ANIM_FRAME(bufInstance, instanceId),
					 INST_ANIM_DURATION(bufInstance, instanceId),
					 bone_index
					);

			animation_transform = (transform * weight) + animation_transform;
		}

		output.position = mul(output.position, animation_transform);

		output.normal   = mul(input.normal, (float3x3)animation_transform);
		output.tangent  = mul(input.tangent, (float3x3)animation_transform);
		output.binormal = mul(input.binormal, (float3x3)animation_transform);
	}

    const matrix world = INST_WORLD(bufInstance, instanceId);
	output.scale       = GetScale(world);

	// Calculate the position of the vertex against the world, view, and
	// projection matrices.
	output.position      = mul(output.position, world);
	output.worldPosition = output.position;

#define PARAM_LOCAL_CUSTOM_VP bufLocalParam[0].iParam[0].w
#define PARAM_LOCAL_CUSTOM_VIEW bufLocalParam[0].mParam[1]
#define PARAM_LOCAL_CUSTOM_PROJ bufLocalParam[0].mParam[2]
	if (PARAM_LOCAL_CUSTOM_VP)
	{
		output.position = mul(output.position, PARAM_LOCAL_CUSTOM_VIEW);
		output.position = mul(output.position, PARAM_LOCAL_CUSTOM_PROJ);
	}
	else
	{
		output.position = mul(output.position, g_camView);
		output.position = mul(output.position, g_camProj);
	}
#undef PARAM_LOCAL_CUSTOM_PROJ
#undef PARAM_LOCAL_CUSTOM_VIEW
#undef PARAM_LOCAL_CUSTOM_VP

	// Store the input color for the pixel shader to use.
	output.color = input.color;
	output.tex   = input.tex;

	[unroll] for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
	{
		const float4 light_position = GetTranslation(bufLight[i].world);
		output.lightDelta[i]        = light_position.xyz - output.worldPosition.xyz;
		output.lightDelta[i]        = output.lightDelta[i];
	}

	const float3 cam_position = GetTranslation(g_camWorld);

	output.viewDirection = cam_position.xyz - output.worldPosition.xyz;
	output.viewDirection = normalize(output.viewDirection);

    output.normal = mul(float4(output.normal, 1.f), world);
    output.tangent = mul(float4(output.tangent, 1.f), world);
    output.binormal = mul(float4(output.binormal, 1.f), world);

	matrix reflectionWorld = mul(g_camReflectView, g_camProj);
	reflectionWorld        = mul(world, reflectionWorld);
	output.reflection      = mul(output.position, reflectionWorld);

	matrix vpw        = mul(g_camView, g_camProj);
	vpw               = mul(world, vpw);
	output.refraction = mul(output.position, vpw);

	output.clipSpacePosZ = output.position.z;
    output.clipPlane = dot(mul(input.position, world), INST_CLIP_PLANE(bufInstance, instanceId));

	output.instanceId = instanceId;

	return output;
}
