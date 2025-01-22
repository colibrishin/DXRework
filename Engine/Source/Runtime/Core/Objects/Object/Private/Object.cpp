#include "../Public/Object.h"
#include "Object.generated.h"

namespace Engine
{
	Object::Object()
		: ObjectBase() { }

	Strong<Abstracts::ObjectBase> Object::cloneImpl() const
	{
		return boost::make_shared<Object>(*this);
	}
}
