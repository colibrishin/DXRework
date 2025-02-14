#pragma once
#include "ObjectBase/Public/ObjectBase.h"

#include "Object.generated.h"

namespace Engine
{
	// Anonymous non-type defined object
	ECLASS(object, serialize)
	class ENGINE_CORE_API Object : public Abstracts::ObjectBase
	{
		GENERATE_BODY
	public:
		OBJECT_T(DEF_OBJ_T_NONE)
		Object();

	};
} // namespace Engine
