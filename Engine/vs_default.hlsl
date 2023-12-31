#include "common.hlsli"

PixelInputType main(VertexInputType input)
{
    PixelInputType output;

    output.position = float4(input.position, 1.0f);

    output.normal   = input.normal;
    output.tangent  = input.tangent;
    output.binormal = input.binormal;
    
    if (input.bone_element.bone_count > 0)
    {
        matrix animation_transform;

        for (int i = 0; i < input.bone_element.bone_count; ++i)
        {
            const int    bone_index     = input.bone_element.boneIndex[i];
            const float  weight         = input.bone_element.boneWeight[i];
            const matrix transform = bufBoneTransform[bone_index].transform;
            animation_transform += transform * weight;
        }

        output.position = mul(output.position, animation_transform);

        output.normal   = mul(input.normal, (float3x3)animation_transform);
        output.tangent  = mul(input.tangent, (float3x3)animation_transform);
        output.binormal = mul(input.binormal, (float3x3)animation_transform);
    }

    // Calculate the position of the vertex against the world, view, and
    // projection matrices.
    output.position       = mul(output.position, g_world);
    output.world_position = output.position;

    output.position = mul(output.position, g_camView);
    output.position = mul(output.position, g_camProj);

    // Store the input color for the pixel shader to use.
    output.color = input.color;
    output.tex   = input.tex;

    [unroll] for (int i = 0; i < g_lightCount; ++i)
    {
        const float4 light_position = GetWorldPosition(g_lightWorld[i]);
        output.lightDirection[i] = light_position.xyz - output.world_position.xyz;
        output.lightDirection[i]    = normalize(output.lightDirection[i]);
    }

    const float3 cam_position = GetWorldPosition(g_camWorld);

    output.viewDirection = cam_position.xyz - output.world_position.xyz;
    output.viewDirection = normalize(output.viewDirection);

    output.normal   = mul(output.normal, (float3x3)g_world);
    output.tangent  = mul(output.tangent, (float3x3)g_world);
    output.binormal = mul(output.binormal, (float3x3)g_world);

    matrix reflectionWorld = mul(g_camReflectView, g_camProj);
    reflectionWorld        = mul(g_world, reflectionWorld);
    output.reflection      = mul(output.position, reflectionWorld);

    matrix vpw        = mul(g_camView, g_camProj);
    vpw               = mul(g_world, vpw);
    output.refraction = mul(output.position, vpw);

    output.clipSpacePosZ = output.position.z;
    output.clipPlane     = dot(mul(input.position, g_world), g_clipPlane);

    return output;
}
