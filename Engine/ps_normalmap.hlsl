#include "common.hlsli"

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    float4 cascadeLocalPosition[3];

	[unroll]
    for (int i = 0; i < 3; ++i)
    {
        cascadeLocalPosition[i] = mul(cam_invProjection, input.position);
        cascadeLocalPosition[i] = mul(cam_invView, cascadeLocalPosition[i]);
        cascadeLocalPosition[i] = mul(cascadeLocalPosition[i], lightFrustumView[i]);
        cascadeLocalPosition[i] = mul(cascadeLocalPosition[i], lightFrustumProj[i]);
    }

    float shadowFactor = 0.f;

	[unroll]
    for (int j = 0; j < MAX_NUM_CASCADES; ++j)
    {
        if (input.clipSpacePosZ <= cascadeEndClipSpace[j].z)
        {
            shadowFactor = GetShadowFactor(j, cascadeLocalPosition[j]);
            break;
        }
    }

    float4 shadow = float4(lerp(float3(0, 0, 0), float3(1, 1, 1), shadowFactor), 1.f);

    const float4 textureColor = shaderTexture.Sample(PSSampler, input.tex);
    float normalLightIntensity[MAX_NUM_LIGHTS];
    float textureLightIntensity[MAX_NUM_LIGHTS];

    const float4 ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

    float4 normalColorArray[MAX_NUM_LIGHTS];
    float4 textureColorArray[MAX_NUM_LIGHTS];

    float4 normalMap = shaderNormalMap.Sample(PSSampler, input.tex);

    normalMap = (normalMap * 2.0f) - 1.0f;

    float3 bumpNormal = (normalMap.x * input.tangent) + (normalMap.y * input.binormal) + (normalMap.z * input.normal);
    bumpNormal = normalize(bumpNormal);

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        normalLightIntensity[i] = saturate(dot(bumpNormal, input.lightDirection[i]));
        textureLightIntensity[i] = saturate(dot(input.normal, input.lightDirection[i]));

        normalColorArray[i] = shadow * lightColor[i] * normalLightIntensity[i];
        textureColorArray[i] = shadow * lightColor[i] * textureLightIntensity[i];
    }

    float4 normalLightColor = ambientColor;

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        normalLightColor.r += normalColorArray[i].r;
        normalLightColor.g += normalColorArray[i].g;
        normalLightColor.b += normalColorArray[i].b;
    }

    float4 textureLightColor = ambientColor;

    for (int i = 0; i < MAX_NUM_LIGHTS; ++i)
    {
        textureLightColor.r += textureColorArray[i].r;
        textureLightColor.g += textureColorArray[i].g;
        textureLightColor.b += textureColorArray[i].b;
    }

    float4 color = saturate(textureLightColor) * saturate(normalLightColor) * textureColor;

    return color;
}
