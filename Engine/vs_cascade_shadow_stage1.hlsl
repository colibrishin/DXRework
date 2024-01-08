#include "common.hlsli"

GeometryShadowInputType main(VertexInputType input)
{
    GeometryShadowInputType output;

    output.position = float4(input.position, 1.0f);

    if (g_bindFlag.boneFlag.x)
    {
        matrix animation_transform;

        for (int i = 0; i < input.bone_element.bone_count; ++i)
        {
            const int bone_index = input.bone_element.boneIndex[i];
            const float weight = input.bone_element.boneWeight[i];
            const matrix transform = bufBoneTransform[bone_index].transform;
            animation_transform += transform * weight;
        }

        output.position = mul(output.position, animation_transform);
    }

    output.position = float4(input.position, 1.f);
    output.position = mul(output.position, g_world);

    return output;
}
