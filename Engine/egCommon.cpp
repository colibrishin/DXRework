#include "pch.hpp"
#include "egObject.hpp"
#include "egScene.hpp"
#include "egLayer.hpp"
#include "egResource.hpp"

namespace Engine
{
	bool ResourcePriorityComparer::operator()(const StrongResource& Left, const StrongResource& Right) const
	{
		if (Left->GetPriority() != Right->GetPriority())
		{
			return Left->GetPriority() < Right->GetPriority();
		}

		return Left->GetID() < Right->GetID();
	}

	bool ComponentPriorityComparer::operator()(const WeakComponent& Left, const WeakComponent& Right) const
	{
		if (Left.lock()->GetPriority() != Right.lock()->GetPriority())
		{
			return Left.lock()->GetPriority() > Right.lock()->GetPriority();
		}

		return Left.lock()->GetID() > Right.lock()->GetID();
	}
}
