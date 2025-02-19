#include "common.hlsli"

// https://wallisc.github.io/rendering/2021/04/18/Fullscreen-Pass.html
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
};

float4 ps_main(float4 input : SV_Position) : SV_Target
{
    float4 worldNormal = float4(0.f, 0.f, 0.f, 1.f);
    float4 baseColor = float4(0.f, 0.f, 0.f, 1.f);
    float metalic = 0.f;
    float roughness = 0.f;
    float ao = 0.f;
    float worldPosition = float4(0.f, 0.f, 0.f, 1.f);
    
    worldNormal = tex[0].Sample(PSSampler, input.xy);
    baseColor = tex[1].Sample(PSSampler, input.xy);
    float4 MRA = tex[2].Sample(PSSampler, input.xy);
    metalic = MRA.x;
    roughness = MRA.y;
    ao = MRA.z;
    worldPosition = tex[3].Sample(PSSampler, input.xy);
    
    // todo: light calculation
    return float4(0, 0, 0, 0);
}