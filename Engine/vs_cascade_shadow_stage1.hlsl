#include "common.hlsli"

GeometryShadowInputType main(VertexInputType input)
{
    GeometryShadowInputType output;

    output.position = float4(input.position, 1.f);
    output.position = mul(output.position, g_world);

    return output;
}
