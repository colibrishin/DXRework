#include "common.hlsli"
#include "pbr.hlsli"

struct DeferredLightPixelInputType
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

// https://wallisc.github.io/rendering/2021/04/18/Fullscreen-Pass.html
DeferredLightPixelInputType vs_main(uint id : SV_VertexID)
{
    DeferredLightPixelInputType output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
};

float4 ps_main(DeferredLightPixelInputType input) : SV_Target
{
    float4 worldNormal = float4(0.f, 0.f, 0.f, 1.f);
    float4 baseColor = float4(0.f, 0.f, 0.f, 1.f);
    float specular = 0.f;
    float metallic = 0.f;
    float roughness = 0.f;
    float ao = 1.f;
    float4 worldPosition = float4(0.f, 0.f, 0.f, 1.f);
    
    worldNormal = float4(tex[0].Sample(PSSampler, input.uv).rgb, 1.f);
    baseColor = float4(tex[1].Sample(PSSampler, input.uv).rgb, 1.f);
    float4 MRAS = tex[2].Sample(PSSampler, input.uv).rgba;
    metallic = MRAS.x;
    roughness = MRAS.y;
    ao = MRAS.z;
    specular = MRAS.w;
    worldPosition = tex[3].Sample(PSSampler, input.uv);
    
    float3 cameraPosition = GetTranslation(g_camWorld).xyz;
    float3 viewDir = normalize(cameraPosition - worldPosition.xyz);
    
    float4 color = float4(PBRLight(viewDir,
                                   worldNormal.xyz,
                                   worldPosition,
                                   baseColor.rgb,
                                   specular,
                                   roughness,
                                   metallic,
                                   ao), 1.f);
    
    return color;
}