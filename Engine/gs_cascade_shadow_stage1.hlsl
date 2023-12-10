#include "common.hlsli"

[maxvertexcount(TRIANGLE_MACRO * MAX_NUM_CASCADES)]
void main(triangle GeometryShadowInputType input[3], inout TriangleStream<PixelShadowStage1InputType> output)
{
    for (int i = 0; i < MAX_NUM_CASCADES; ++i)
    {
        PixelShadowStage1InputType element;
        element.RTIndex = i;

        for (int j = 0; j < TRIANGLE_MACRO; ++j)
        {
            float4 posV = mul(input[j].position, lightFrustumView[i]);
            posV.z += 2.5f;
            element.position = mul(posV, lightFrustumProj[i]);
            element.tex = input[i].tex;
            output.Append(element);
        }

        output.RestartStrip();
    }
}