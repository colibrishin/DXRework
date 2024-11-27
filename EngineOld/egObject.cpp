#include "pch.h"
#include "egObject.hpp"

SERIALIZE_IMPL
(
 Engine::Object,
 _ARTAG(_BSTSUPER(Engine::Abstract::ObjectBase))
)

namespace Engine
{
	Object::Object()
		: ObjectBase() { }

	StrongObjectBase Object::cloneImpl() const
	{
		return boost::make_shared<Object>(*this);
	}
}
