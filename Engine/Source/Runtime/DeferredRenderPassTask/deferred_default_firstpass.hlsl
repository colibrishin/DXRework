#include "common.hlsli"
#include "vs_default.hlsl"

struct DeferredOutput
{
    float4 A : SV_Target0;
    float4 B : SV_Target1;
    float4 C : SV_Target2;
    float4 D : SV_Target3;
};

DeferredOutput ps_main(PixelInputType input)
{
    DeferredOutput output;
    float4 worldNormal = float4(input.normal, 1.f);
    float4 baseColor = input.color;
    float metallic = 0.f;
    float roughness = 0.f;
    float ao = 0.f;
    
    if (INST_TEX_SLOT0_ENABLE(bufInstance, input.instanceId) == true)
    {
        const float3 localNormal = BiasX2(Sample(PSSampler, input.tex, INST_TEX_SLOT0(bufInstance, input.instanceId)).rgb);
        worldNormal = float4(PeturbNormal(localNormal, input.worldPosition.xyz, input.normal, input.tex), 1.f);
    }
    if (INST_TEX_SLOT1_ENABLE(bufInstance, input.instanceId) == true)
    {
        baseColor = Sample(PSSampler, input.tex, INST_TEX_SLOT1(bufInstance, input.instanceId));
    }
    if (INST_TEX_SLOT2_ENABLE(bufInstance, input.instanceId) == true)
    {
        float4 mra = Sample(PSSampler, input.tex, INST_TEX_SLOT2(bufInstance, input.instanceId));
        metallic = mra.r;
        roughness = mra.g;
        ao = mra.b;
    }

    output.A = worldNormal;
    output.B = baseColor;
    output.C = float4(metallic, roughness, ao, 1.f);
    output.D = input.worldPosition;
    return output;
}