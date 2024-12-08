#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
	int i = 0;

	float shadowFactor[MAX_NUM_LIGHTS];
	GetShadowFactor(input.worldPosition, input.clipSpacePosZ, shadowFactor);

	if (bufMaterial[0].repeatMaterial.x == true)
	{
		const float2 scaleWiseTex = input.tex * input.scale.xy;
		const float2 repeatTex    = frac(scaleWiseTex);
		input.tex                 = repeatTex;
	}

	const float4 textureColor = tex00.Sample(PSSampler, input.tex);

	float  lightIntensity[MAX_NUM_LIGHTS];
	float4 colorArray[MAX_NUM_LIGHTS];

	for (i = 0; i < PARAM_NUM_LIGHT; ++i)
	{
		const float dist = length(input.lightDelta[i]);

		if (bufLight[i].type.x == LIGHT_TYPE_SPOT)
		{
			if (dist > bufLight[i].range.x)
			{
				lightIntensity[i] = 0.f;
				colorArray[i]     = LerpShadow(shadowFactor[i]) * g_ambientColor;
				continue;
			}
		}

		const float3 light_dir = normalize(input.lightDelta[i]);

		lightIntensity[i] = saturate(dot(input.normal, light_dir));
		colorArray[i]     =
				LerpShadow(shadowFactor[i]) * bufLight[i].color * lightIntensity[i];

		if (bufLight[i].type.x == LIGHT_TYPE_SPOT)
		{
			lightIntensity[i] *= saturate(1.0f - (dist / bufLight[i].range.x));
		}
	}

	float4 colorSum = g_ambientColor;

	for (i = 0; i < PARAM_NUM_LIGHT; ++i)
	{
		colorSum.r += colorArray[i].r;
		colorSum.g += colorArray[i].g;
		colorSum.b += colorArray[i].b;
	}

	float4 color = saturate(colorSum) * textureColor;

	return color;
}
