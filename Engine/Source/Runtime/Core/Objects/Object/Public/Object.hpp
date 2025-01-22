#pragma once
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"

namespace Engine
{
	class Object;
}

POLYMORPHIC_TYPE_MAP(ENGINE_CORE_API, Engine::Object, Engine::Abstracts::ObjectBase)

namespace Engine
{
	// Anonymous non-type defined object
	class ENGINE_CORE_API Object final : public Abstracts::ObjectBase
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Object)
		OBJECT_T(DEF_OBJ_T_NONE)
		Object();

	protected:
		Strong<Abstracts::ObjectBase> cloneImpl() const override;

	};
} // namespace Engine
