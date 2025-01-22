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
        MaterialBindFlag flags;

        EPROPERTY()
        float specularPower;
        EPROPERTY()
        float reflectionTranslation;
        EPROPERTY()
        float reflectionScale;
        EPROPERTY()
        float refractionScale;

        EPROPERTY()
        Color        overrideColor;
        EPROPERTY()
        Color        specularColor;
        EPROPERTY()
        Vector4      clipPlane;
        EPROPERTY()
        OffsetT<int> repeatTexture;
    };
}
