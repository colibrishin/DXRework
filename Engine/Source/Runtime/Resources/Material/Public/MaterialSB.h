#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"
#include "MaterialBindFlag.h"
#include "MaterialSB.generated.h"

namespace Engine::Graphics::SBs
{
    ECLASS(serialize)
    struct ENGINE_MATERIAL_API MaterialSB
    {
        GENERATE_BODY
        SB_T(SB_TYPE_MATERIAL)

        EPROPERTY()
        float specularPower = 100.f;
        EPROPERTY()
        float reflectionTranslation = 0.5f;
        EPROPERTY()
        float reflectionScale = 0.15f;
        EPROPERTY()
        float refractionScale = 0.15f;

        EPROPERTY()
        Color        overrideColor;
        EPROPERTY()
        Color        specularColor = {1.f, 1.f, 1.f, 1.f};
        EPROPERTY()
        Vector4      clipPlane = {0.f, 0.f, 0.f, 0.f};
        EPROPERTY()
        OffsetT<int> repeatTexture = false;
    };
}
