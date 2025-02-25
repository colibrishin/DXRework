#include "BoundingGetter.h"

#include "ObjectBase.h"
#include "GenericBounding.hpp"
#include "Transform.h"
#include "Collider.h"

namespace Engine
{
    GenericBounding<> bounding_getter::value(const Weak<Abstracts::ObjectBase>& object)
	{
		const auto tr = object.lock()->GetComponent<Components::Transform>().lock();

		if (const auto cldr = object.lock()->GetComponent<Components::Collider>().lock())
		{
			auto bounding = cldr->GetBounding();
			return bounding;
		}

		GenericBounding bounding;
		bounding.SetType(BOUNDING_TYPE_BOX);
		bounding.Transform(tr->GetWorldMatrix());
		return bounding;
	}
}
