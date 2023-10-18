/////////////
// GLOBALS //
/////////////
cbuffer PerspectiveBuffer
{
	matrix view;
	matrix projection;
};

cbuffer TransformBuffer
{
	matrix scale;
	matrix rotation;
	matrix translation;
};

//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 tex : TEXCOORD0;
};

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

	output.normal = mul(input.normal, (float3x3)world);
	output.normal = normalize(output.normal);

	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);

	// Store the input color for the pixel shader to use.
	output.color = input.color;
	output.tex = input.tex;

	return output;
}
