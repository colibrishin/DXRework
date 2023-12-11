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
            element.position = mul(posV, lightFrustumProj[i]);
            output.Append(element);
        }

        output.RestartStrip();
    }
}