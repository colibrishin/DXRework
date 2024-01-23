#include "common.hlsli"

#define TARGET_SHADOW_PARAM g_iParam[1]

struct PixelShadowInputType
{
  float4 position : SV_POSITION;
  uint   RTIndex : SV_RenderTargetArrayIndex;
};

struct GeometryShadowInputType
{
  float4 position : SV_POSITION;
};

GeometryShadowInputType vs_main(VertexInputType input)
{
  GeometryShadowInputType output;

  output.position = float4(input.position, 1.0f);

  if (g_bindFlag.boneFlag.x)
  {
    matrix animation_transform;

    for (int i = 0; i < input.bone_element.bone_count; ++i)
    {
      const int    bone_index = input.bone_element.boneIndex[i];
      const float  weight     = input.bone_element.boneWeight[i];
      const matrix transform  = bufBoneTransform[bone_index].transform;
      animation_transform += transform * weight;
    }

    output.position = mul(output.position, animation_transform);
  }

  output.position = float4(input.position, 1.f);
  output.position = mul(output.position, g_world);

  return output;
}

[maxvertexcount(TRIANGLE_MACRO * MAX_NUM_CASCADES)]
void gs_main(
  triangle GeometryShadowInputType           input[3],
  inout TriangleStream<PixelShadowInputType> output
)
{
  for (int i = 0; i < MAX_NUM_CASCADES; ++i)
  {
    PixelShadowInputType element;
    element.RTIndex = i;

    for (int j = 0; j < TRIANGLE_MACRO; ++j)
    {
      element.position =
        mul
        (
         input[j].position, mul
         (
          bufLightVP[TARGET_SHADOW_PARAM].g_shadowView[i],
          bufLightVP[TARGET_SHADOW_PARAM].g_shadowProj[i]
         )
        );
      output.Append(element);
    }

    output.RestartStrip();
  }
}


void ps_main(PixelShadowInputType input)
{
  // Passing through
}
