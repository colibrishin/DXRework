#pragma once

#include "egConstant.h"
#include "egType.h"
#include "egDXType.h"
#include "egMacro.h"

namespace Engine::Graphics
{
    namespace CBs
    {
        // Constant buffers
        struct PerspectiveCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_WVP)

            Matrix world;
            Matrix view;
            Matrix projection;

            Matrix invView;
            Matrix invProj;

            Matrix reflectView;
        };

        struct TransformCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_TRANSFORM)

            Matrix world;
        };

        struct LightCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_LIGHT)

            Matrix world[g_max_lights];
            Color  color[g_max_lights];
            int    light_count;
            float  ___p[3];
        };

        struct ShadowVPCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_SHADOW)

            ShadowVP value;
        };

        struct ShadowVPChunkCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_SHADOW_CHUNK)

            ShadowVP lights[g_max_lights];
        };

        struct MaterialCB
        {
            INTERNAL_CB_CHECK_CONSTEXPR(CB_TYPE_MATERIAL)

            float specular_power;
            float reflection_translation;
            float reflection_scale;
            float refraction_scale;

            Color   override_color;
            Color   specular_color;
            Vector4 clip_plane;
        };
    }
} // namespace Engine
