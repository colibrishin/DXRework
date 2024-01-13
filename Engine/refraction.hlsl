#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
    float2 reflectTex;
    reflectTex.x = input.reflection.x / input.reflection.w / 2.0f + 0.5f;
    reflectTex.y = -input.reflection.y / input.reflection.w / 2.0f + 0.5f;

    float2 refractTex;
    refractTex.x = input.refraction.x / input.reflection.w / 2.0f + 0.5f;
    refractTex.y = -input.refraction.y / input.reflection.w / 2.0f + 0.5f;

    float4 normalMap = tex00.Sample(PSSampler, input.tex);
    float3 normal    = (normalMap.xyz * 2.0f) - 1.0f;

    reflectTex += normal.xy * g_reflectionScale;
    refractTex += normal.xy * g_refractionScale;

    const float4 reflectColor = texRendered.Sample(PSSampler, reflectTex);
    const float4 refractColor = texRendered.Sample(PSSampler, refractTex);

    return lerp(reflectColor, refractColor, 0.6f);
}
