#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"
#include "MaterialPrimitive.generated.h"

namespace Engine::Graphics
{
    ECLASS(serialize)
    struct ENGINE_MATERIAL_API MaterialPrimitive
    {
        GENERATE_BODY

        EPROPERTY()
        float specularPower = 100.f;
        EPROPERTY()
        float reflectionTranslation = 0.5f;
        EPROPERTY()
        float reflectionScale = 0.15f;
        EPROPERTY()
        float refractionScale = 0.15f;

        EPROPERTY()
        Color        overrideColor = {0.f, 0.f, 0.f, 0.f};
        EPROPERTY()
        Color        specularColor = {0.f, 0.f, 0.f, 1.f};
        EPROPERTY()
        Vector4      clipPlane = {0.f, 0.f, 0.f, 0.f};

        EPROPERTY()
        int repeatTexture = false;
        EPROPERTY()
        int atlas = false;
        EPROPERTY()
        int texSlot[ g_max_texture_per_material ]{};
        EPROPERTY()
        int texEnabled[ g_max_texture_per_material ]{};

        void Apply(SBs::InstanceSB& instance) const;
    };
}
