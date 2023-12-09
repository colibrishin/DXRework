#include "common.hlsli"

PixelShadowStage2InputType main(VertexInputType input)
{
    PixelShadowStage2InputType output;

    const matrix world = mul(mul(scale, rotation), translation);

    output.position = float4(input.position, 1.f);

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(output.position, world);
    output.position = mul(output.position, cam_view);
    output.position = mul(output.position, cam_projection);

    // Store the input color for the pixel shader to use.
    output.tex = input.tex;
    output.clipSpacePosZ = output.position.z;

    return output;
}