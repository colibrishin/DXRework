#include "common.hlsli"

GeometryShadowInputType main(VertexInputType input)
{
    GeometryShadowInputType output;

    const matrix world = mul(mul(g_scale, g_rotation), g_translation);

    output.position = float4(input.position, 1.f);
    output.position = mul(output.position, world);

    return output;
}
