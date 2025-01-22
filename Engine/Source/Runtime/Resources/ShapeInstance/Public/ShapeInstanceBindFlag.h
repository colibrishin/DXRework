#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"

#include "ShapeInstanceBindFlag.generated.h"

namespace Engine::Graphics::SBs 
{
    ECLASS(serialize)
    struct ENGINE_MATERIAL_API ShapeInstanceBindFlag
    {
        GENERATE_BODY
        
        EPROPERTY()
        OffsetT<int> tex[CFG_PER_PARAM_BUFFER_SIZE];
        EPROPERTY()
        OffsetT<int> texArr[CFG_PER_PARAM_BUFFER_SIZE];
        EPROPERTY()
        OffsetT<int> texCube[CFG_PER_PARAM_BUFFER_SIZE];
        EPROPERTY()
        OffsetT<int> bone;
        EPROPERTY()
        OffsetT<int> atlas;
    };
}
