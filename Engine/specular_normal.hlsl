#include "common.hlsli"
#include "vs_default.hlsl"

float4 ps_main(PixelInputType input) : SV_TARGET
{
  int i = 0;

  float shadowFactor[MAX_NUM_LIGHTS];
  GetShadowFactor(input.world_position, input.clipSpacePosZ, shadowFactor);

  const float4 textureColor = tex00.Sample(PSSampler, input.tex);
  float        normalLightIntensity[MAX_NUM_LIGHTS];
  float        textureLightIntensity[MAX_NUM_LIGHTS];

  float4 normalColorArray[MAX_NUM_LIGHTS];
  float4 textureColorArray[MAX_NUM_LIGHTS];
  float3 reflection[MAX_NUM_LIGHTS];
  float4 specular[MAX_NUM_LIGHTS];

  float4 normalMap = tex01.Sample(PSSampler, input.tex);

  normalMap = (normalMap * 2.0f) - 1.0f;

  float3 bumpNormal = (normalMap.x * input.tangent) +
                      (normalMap.y * input.binormal) +
                      (normalMap.z * input.normal);
  bumpNormal = normalize(bumpNormal);

  for (i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    const float dist = length(input.lightDelta[i]);

    if (bufLight[i].type.x == LIGHT_TYPE_SPOT)
    {
      if (dist > bufLight[i].range)
      {
        normalColorArray[i] = 0.f;
        textureColorArray[i] = 0.f;
        normalColorArray[i]  = LerpShadow(shadowFactor[i]) * g_ambientColor;
        textureColorArray[i] = LerpShadow(shadowFactor[i]) * g_ambientColor;
        continue;
      }
    }

    const float3 light_dir = normalize(input.lightDelta[i]);

    textureLightIntensity[i] =
      saturate(dot(input.normal, light_dir));
    normalLightIntensity[i] =
      saturate(dot(bumpNormal, light_dir));

    const float4 shadow = LerpShadow(shadowFactor[i]);

    normalColorArray[i]  = shadow * bufLight[i].color * normalLightIntensity[i];
    textureColorArray[i] = shadow * bufLight[i].color * textureLightIntensity[i];

    reflection[i] = normalize
      (
       2.0f * normalLightIntensity[i] * input.normal - light_dir
      );
    specular[i] =
      pow(saturate(dot(reflection[i], input.viewDirection)), g_specularPower);

    if (bufLight[i].type.x == LIGHT_TYPE_SPOT)
    {
      normalColorArray[i] *= saturate(1.0f - dist / bufLight[i].range);
      textureColorArray[i] *= saturate(1.0f - dist / bufLight[i].range);
    }
  }

  float4 normalLightColor = g_ambientColor;

  for (i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    normalLightColor.r += normalColorArray[i].r;
    normalLightColor.g += normalColorArray[i].g;
    normalLightColor.b += normalColorArray[i].b;
  }

  float4 textureLightColor = g_ambientColor;

  for (i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    textureLightColor.r += textureColorArray[i].r;
    textureLightColor.g += textureColorArray[i].g;
    textureLightColor.b += textureColorArray[i].b;
  }

  float4 specularSum = float4(0.0f, 0.0f, 0.0f, 1.0f);

  for (i = 0; i < PARAM_NUM_LIGHT; ++i)
  {
    specularSum.r += specular[i].r;
    specularSum.g += specular[i].g;
    specularSum.b += specular[i].b;
  }

  float4 color =
    saturate(textureLightColor) * saturate(normalLightColor) * textureColor +
    specularSum;

  return color;
}
