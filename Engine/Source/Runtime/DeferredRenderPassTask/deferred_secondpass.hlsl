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
    float specular = 0.f;
    float metallic = 0.f;
    float roughness = 0.f;
    float ao = 1.f;
    float4 worldPosition = float4(0.f, 0.f, 0.f, 1.f);
    
    worldNormal = tex[0].Sample(PSSampler, input.uv);
    baseColor = tex[1].Sample(PSSampler, input.uv);
    float4 MRAS = tex[2].Sample(PSSampler, input.uv);
    metallic = MRAS.r;
    roughness = MRAS.g;
    ao = MRAS.b;
    specular = MRAS.a;
    worldPosition = tex[3].Sample(PSSampler, input.uv);
    
    float4 lightIntensity[MAX_NUM_LIGHTS];
    float4 colorArray[MAX_NUM_LIGHTS];
    float4 specularArray[MAX_NUM_LIGHTS];    
    float shadowFactor[MAX_NUM_LIGHTS];
    
    const float4 wvp = mul(mul(worldPosition, g_camView), g_camProj);
    GetShadowFactor(worldPosition, wvp.z, shadowFactor);
    
    // todo: MR
    for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
    {
        const float4 lightPos = GetTranslation(bufLight[i].world);
        const float4 lightDir = float4(normalize(worldPosition - lightPos).xyz, 1.f);
        const float4 shadow = LerpShadow(shadowFactor[i]);
        
        lightIntensity[i] = saturate(dot(worldNormal, lightDir));

        const float4 normalLightDir = float4(normalize(worldNormal - lightPos).xyz, 1.f);
        const float4 viewDir = normalize(GetTranslation(g_camWorld) - worldPosition);
        
        const float4 reflection = normalize(2.0f * lightIntensity[i] * normalLightDir);
        const float4 specularFactor = pow(saturate(dot(reflection, viewDir)), specular);
        
        colorArray[i] = shadow * bufLight[i].color * lightIntensity[i] * ao;
        specularArray[i] = specularFactor * ao;
    }
    
    float3 lightColor = g_ambientColor;
    for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
    {
        lightColor.r += colorArray[i].r;
        lightColor.g += colorArray[i].g;
        lightColor.b += colorArray[i].b;
    }
    
    float3 specularColor = float3(0, 0, 0);
    for (int i = 0; i < PARAM_NUM_LIGHT; ++i)
    {
        specularColor.r += specularArray[i].r;
        specularColor.g += specularArray[i].g;
        specularColor.b += specularArray[i].b;
    }
    
    return float4(saturate(lightColor) * baseColor.rgb + specularColor, 1.f);
}