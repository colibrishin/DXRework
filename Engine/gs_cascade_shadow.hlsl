#include "common.hlsli"

[maxvertexcount(9)]
void main (triangle GeometryInputType input[3], inout TriangleStream<GeometryOutputType> output)
{
    for (int face = 0; face < 3; ++face)
    {
        GeometryOutputType element;
        element.RTIndex = face;

        for (int j = 0; j < 3; ++j)
        {
            element.position = mul(lightWorld[face], input[face].position);
            element.tex = input[face].tex;
            output.Append(element);
        }

        output.RestartStrip();
    }
}