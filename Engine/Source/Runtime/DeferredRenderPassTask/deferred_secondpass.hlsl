#include "common.hlsli"

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
    float metallic = 0.f;
    float roughness = 0.f;
    float ao = 0.f;
    float4 worldPosition = float4(0.f, 0.f, 0.f, 1.f);
    
    worldNormal = tex[0].Sample(PSSampler, input.uv);
    baseColor = tex[1].Sample(PSSampler, input.uv);
    float4 MRA = tex[2].Sample(PSSampler, input.uv);
    metallic = MRA.x;
    roughness = MRA.y;
    ao = MRA.z;
    worldPosition = tex[3].Sample(PSSampler, input.uv);
    
    float4 lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];
    
    float shadowFactor[MAX_NUM_LIGHTS];
    GetShadowFactor(worldPosition, worldPosition.z, shadowFactor);
    
    // todo: MRA
    for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
    {
        const float4 lightPos = GetTranslation(bufLight[i].world);
        const float3 lightDir = normalize(worldPosition - lightPos).xyz;
        const float4 shadow = LerpShadow(shadowFactor[i]);
        
        lightIntensity[i] = saturate(dot(worldNormal.xyz, lightDir));
        colorArray[i] = shadow * bufLight[i].color * lightIntensity[i];
    }
    
    float4 lightColor = g_ambientColor;
    for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
    {
        lightColor.r += colorArray[i].r;
        lightColor.g += colorArray[i].g;
        lightColor.b += colorArray[i].b;
    }
    
    float4 color = saturate(lightColor) * baseColor;
    return color;
}