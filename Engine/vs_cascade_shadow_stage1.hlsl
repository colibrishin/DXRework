#include "common.hlsli"

GeometryShadowInputType main(VertexInputType input)
{
	GeometryShadowInputType output;

	const matrix world = mul(mul(scale, rotation), translation);

	output.position = float4(input.position, 1.f);
	output.position = mul(output.position, world);

	return output;
}