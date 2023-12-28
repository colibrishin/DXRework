#include "common.hlsli"

[maxvertexcount(TRIANGLE_MACRO * MAX_NUM_CASCADES)] void main(
    triangle GeometryShadowInputType                 input[3],
    inout TriangleStream<PixelShadowInputType> output)
{
    for (int i = 0; i < MAX_NUM_CASCADES; ++i)
    {
        PixelShadowInputType element;
        element.RTIndex = i;

        for (int j = 0; j < TRIANGLE_MACRO; ++j)
        {
            element.position =
                    mul(
                        input[j].position, mul(
                                               g_currentShadow.g_shadowView[i],
                                               g_currentShadow.g_shadowProj[i]));
            output.Append(element);
        }

        output.RestartStrip();
    }
}
