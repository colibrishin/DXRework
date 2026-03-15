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
    };
} // namespace Engine
