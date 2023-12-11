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
	output.world_position = output.position;

	output.position = mul(output.position, cam_view);
	output.position = mul(output.position, cam_projection);

	// Store the input color for the pixel shader to use.
	output.color = input.color;
	output.tex = input.tex;

    float4 worldPosition = mul(input.position, world);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        const float4 light_position = GetWorldPosition(lightWorld[i]);
        output.lightDirection[i] = light_position.xyz - worldPosition.xyz;
        output.lightDirection[i] = normalize(output.lightDirection[i]);
    }

    const float3 cam_position = GetWorldPosition(cam_world);

    output.viewDirection = cam_position.xyz - worldPosition.xyz;
	output.viewDirection = normalize(output.viewDirection);

    output.normal = mul(input.normal, (float3x3)world);
	output.tangent = mul(input.tangent, (float3x3)world);
	output.binormal = mul(input.binormal, (float3x3)world);

    output.clipSpacePosZ = output.position.z;

    return output;
}
