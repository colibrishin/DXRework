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
    // Nothing rasterized here (the normal is cleared to 0): keep what is under it (the ray traced image, or the clear).
    if (dot(worldNormal.rgb, worldNormal.rgb) == 0.f)
    {
        discard;
    }
    baseColor = tex[1].Sample(PSSampler, input.uv);
    float4 MRAS = tex[2].Sample(PSSampler, input.uv);
    metallic = MRAS.r;
    roughness = MRAS.g;
    ao = MRAS.b;
    specular = MRAS.a;
    worldPosition = tex[3].Sample(PSSampler, input.uv);
    
    float shadowFactor[MAX_NUM_LIGHTS];
    
    const float4 wvp = mul(mul(worldPosition, g_camView), g_camProj);
    GetShadowFactor(worldPosition, wvp.z, shadowFactor);
    
    // Blinn-Phong over the G-buffer (non-PBR fallback). Directions are float3 so the w=1 components
    // do not leak into the dot products.
    const float3 N       = normalize(worldNormal.xyz);
    const float3 viewDir = normalize(GetTranslation(g_camWorld).xyz - worldPosition.xyz);

    float3 lightColor    = g_ambientColor.rgb;
    float3 specularColor = float3(0, 0, 0);

    for (int li = 0; li < PARAM_NUM_LIGHT; ++li)
    {
        const float3 toLight   = normalize(GetTranslation(bufLight[li].world).xyz - worldPosition.xyz);
        const float  shadow    = LerpShadow(shadowFactor[li]).r;
        const float  intensity = saturate(dot(N, toLight));
        const float3 halfVec   = normalize(toLight + viewDir);
        const float  spec      = intensity > 0.f ? pow(saturate(dot(N, halfVec)), max(specular * 255.f, 1.f)) : 0.f;

        lightColor    += shadow * bufLight[li].color.rgb * intensity * ao;
        specularColor += shadow * bufLight[li].color.rgb * spec * ao;
    }

    return float4(saturate(lightColor) * baseColor.rgb + specularColor, 1.f);
}