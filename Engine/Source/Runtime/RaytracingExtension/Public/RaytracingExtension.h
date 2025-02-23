#pragma once
#if CFG_RAYTRACING
#include "CoreType.h"

#include "RaytracingExtension.generated.h"

namespace Engine
{
    ECLASS()
    struct ENGINE_RAYTRACINGEXTENSION_API RaytracingExtension
    {
        GENERATE_BODY
        static void SetRaytracing(const bool flag);
    };
}
#endif