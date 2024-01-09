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
                                               bufLightVP[g_targetShadow.x].g_shadowView[i],
                                               bufLightVP[g_targetShadow.x].g_shadowProj[i]));
            output.Append(element);
        }

        output.RestartStrip();
    }
}
