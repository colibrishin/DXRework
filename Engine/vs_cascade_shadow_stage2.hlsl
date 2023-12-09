#include "common.hlsli"

PixelShadowStage2InputType main(VertexInputType input)
{
    PixelShadowStage2InputType output;

    const matrix world = mul(mul(scale, rotation), translation);

    output.position = float4(input.position, 1.f);

    // applying only world for transform to the light frustum local space
	output.position = mul(output.position, world);

    // Store the input color for the pixel shader to use.
    output.tex = input.tex;
    output.clipSpacePosZ = output.position.z;

    return output;
}