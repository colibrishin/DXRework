#include "Source/Runtime/BoundingGetter/Public/BoundingGetter.h"

#include "Source/Runtime/Abstracts/CoreObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/GenericBounding/Public/GenericBounding.hpp"
#include "Source/Runtime/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Components/Collider/Public/Collider.hpp"

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
