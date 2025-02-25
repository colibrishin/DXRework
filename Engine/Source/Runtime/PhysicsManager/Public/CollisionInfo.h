#pragma once
#include "TypeLibrary.h"

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
