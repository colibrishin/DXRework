#pragma once
#include "TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
    struct CollisionInfo
    {
        Weak<Abstracts::ObjectBase> lhs;
        Weak<Abstracts::ObjectBase> rhs;

        bool speculative;
        bool collision;
    };
}
