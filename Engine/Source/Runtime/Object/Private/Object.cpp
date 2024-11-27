#include "Source/Runtime/Object/Public/Object.hpp"

SERIALIZE_IMPL
(
 Engine::Object,
 _ARTAG(_BSTSUPER(Engine::Abstracts::ObjectBase))
)

namespace Engine
{
	Object::Object()
		: ObjectBase() { }

	Strong<Abstracts::ObjectBase> Object::cloneImpl() const
	{
		return boost::make_shared<Object>(*this);
	}
}
