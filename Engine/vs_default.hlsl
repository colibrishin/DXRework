#include "common.hlsli"

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	// Change the position vector to be 4 units for proper matrix calculations.
	output.position = float4(input.position, 1.0f);

	matrix world = mul(mul(scale, rotation), translation);

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(output.position, world);
	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);

	// Store the input color for the pixel shader to use.
	output.color = input.color;
	output.tex = input.tex;

    float4 worldPosition = mul(input.position, world);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        output.lightDirection[i] = lightPosition[i].xyz - worldPosition.xyz;
        output.lightDirection[i] = normalize(output.lightDirection[i]);
    }

    const float3 cam_position = float3(cam_world._41, cam_world._42, cam_world._43);

    output.viewDirection = cam_position.xyz - worldPosition.xyz;
	output.viewDirection = normalize(output.viewDirection);

    output.normal = mul(input.normal, (float3x3)world);
	output.tangent = mul(input.tangent, (float3x3)world);
	output.binormal = mul(input.binormal, (float3x3)world);

    return output;
}
