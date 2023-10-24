/////////////
// GLOBALS //
/////////////
#define MAX_NUM_LIGHTS 8

cbuffer PerspectiveBuffer : register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer TransformBuffer : register(b1)
{
	matrix scale;
	matrix rotation;
	matrix translation;
};

cbuffer LightPositionBuffer : register(b2)
{
    float4 lightPosition[MAX_NUM_LIGHTS];
}

//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 tex : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 tex : TEXCOORD0;
    float3 lightPos[MAX_NUM_LIGHTS] : TEXCOORD1;
    float3 tangent : TANGENT;
    float3 binormal : BINOARML;
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

	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);

	// Store the input color for the pixel shader to use.
	output.color = input.color;
	output.tex = input.tex;

    float4 worldPosition = mul(input.position, world);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        output.lightPos[i] = lightPosition[i].xyz - worldPosition.xyz;
        output.lightPos[i] = normalize(output.lightPos[i]);
    }

    output.normal = mul(input.normal, (float3x3)world);
	output.tangent = mul(input.tangent, (float3x3)world);
	output.binormal = mul(input.binormal, (float3x3)world);

    return output;
}
