#include "pch.hpp"

namespace Engine
{
	bool ResourcePriorityComparer::operator()(const WeakResource& Left, const WeakResource& Right) const
	{
		if (Left.lock()->GetPriority() != Right.lock()->GetPriority())
		{
			return Left.lock()->GetPriority() < Right.lock()->GetPriority();
		}

		return Left.lock()->GetID() < Right.lock()->GetID();
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
