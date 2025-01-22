#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"

#include "ShapeInstanceBindFlag.generated.h"

namespace Engine::Graphics 
{
    ECLASS(serialize)
    struct ENGINE_MATERIAL_API ShapeInstanceBindFlag
    {
        GENERATE_BODY

        EPROPERTY()
        int bone;
    };
}
