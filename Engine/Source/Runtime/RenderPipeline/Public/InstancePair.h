#pragma once
#include "TypeLibrary.h"

namespace Engine
{
    struct InstancePair
    {
        Strong<Abstracts::ObjectBase>                                                           object;
        Graphics::SBs::InstanceSB*                                                              instance;
        std::array<Strong<Abstracts::Resource>, g_max_texture_per_material>                      textures;
        std::array<Strong<Abstracts::Resource>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> reservedTextures;
        // Non-zero: instance points at this many contiguous instances that share the object and textures (a particle
        // emitter), drawn by this one entry.
        size_t                                                                                  instanceCount = 0;
    };
} // namespace Engine
