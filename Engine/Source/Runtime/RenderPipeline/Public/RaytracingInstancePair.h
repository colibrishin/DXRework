#pragma once
#include "IGraphicAPI.h"
#include "TypeLibrary.h"

namespace Engine
{
    struct ENGINE_RENDERPIPELINE_API RaytracingInstancePair
    {
        std::array<Strong<Abstracts::Resource>, g_max_texture_per_material> textures;
        StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>               instance;
    };
}
