#include "pch.h"
#include "egLayer.h"
#include "egObject.hpp"
#include "egResource.h"
#include "egScene.hpp"

namespace Engine
{
    bool ResourcePriorityComparer::operator()(
        const StrongResource& Left,
        const StrongResource& Right) const
    {
        if (Left->GetPriority() != Right->GetPriority())
        {
            return Left->GetPriority() < Right->GetPriority();
        }

        return Left->GetID() < Right->GetID();
    }

    bool ComponentPriorityComparer::operator()(
        const WeakComponent& Left,
        const WeakComponent& Right) const
    {
        if (Left.lock()->GetPriority() != Right.lock()->GetPriority())
        {
            return Left.lock()->GetPriority() > Right.lock()->GetPriority();
        }

        return Left.lock()->GetID() > Right.lock()->GetID();
    }
} // namespace Engine
