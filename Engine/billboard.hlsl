#include "common.hlsli"

struct PixelBillboardInputType
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 tex : TEXCOORD0;
};

struct GeometryBillboardInputType
{
  float4 position : POSITION;
  uint   InstanceID : SV_InstanceID;
};

GeometryBillboardInputType vs_main(VertexInputType input, uint instanceId : SV_InstanceID)
{
  GeometryBillboardInputType output;

  output.position = float4(input.position, 1.0f);
#define INST_WORLD     mParam[0]
  output.position = mul(output.position, bufInstance[instanceId].INST_WORLD);
#undef INST_WORLD
  output.InstanceID = instanceId;

  return output;
};

[maxvertexcount(6)]
void gs_main(
  point GeometryBillboardInputType input[1], 
  inout TriangleStream<PixelBillboardInputType> OutputStream)
{
  PixelBillboardInputType output[6];

#define INST_WORLD mParam[0]
  const float4 worldPos = GetTranslation(bufInstance[input[0].InstanceID].INST_WORLD);
  const float3 scale    = GetScale(bufInstance[input[0].InstanceID].INST_WORLD);
  const float4 viewPos  = mul(worldPos, g_camView);
#undef INST_WORLD

  float4 baseSquare[4] =
  {
    float4(-0.5f, 0.5f, 0.0f, 1.0f),
    float4(0.5f, 0.5f, 0.0f, 1.0f),
    float4(0.5f, -0.5f, 0.0f, 1.0f),
    float4(-0.5f, -0.5f, 0.0f, 1.0f)
  };

  for (int i = 0; i < 4; ++i) { baseSquare[i].xyz *= scale; }

  const float2 texCoords[4] =
  {
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f)
  };

  output[0].position = mul(baseSquare[0] + viewPos, g_camProj);
  output[1].position = mul(baseSquare[1] + viewPos, g_camProj);
  output[2].position = mul(baseSquare[2] + viewPos, g_camProj);
  output[3].position = mul(baseSquare[3] + viewPos, g_camProj);

  output[0].tex = texCoords[0];
  output[1].tex = texCoords[1];
  output[2].tex = texCoords[2];
  output[3].tex = texCoords[3];

  output[0].color = float4(1.0f, 0.0f, 0.0f, 1.0f);
  output[1].color = float4(0.0f, 1.0f, 0.0f, 1.0f);
  output[2].color = float4(0.0f, 0.0f, 1.0f, 1.0f);
  output[3].color = float4(1.0f, 0.0f, 1.0f, 1.0f);

  OutputStream.Append(output[0]);
  OutputStream.Append(output[2]);
  OutputStream.Append(output[3]);
  OutputStream.RestartStrip();

  OutputStream.Append(output[0]);
  OutputStream.Append(output[1]);
  OutputStream.Append(output[2]);
  OutputStream.RestartStrip();
};

float4 ps_main(PixelBillboardInputType input) : SV_TARGET
{
  const float4 tex = tex00.Sample(PSSampler, input.tex);
  return tex;
};