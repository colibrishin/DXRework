#pragma once
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Object.generated.h"

namespace Engine
{
	// Anonymous non-type defined object
	ECLASS()
	class ENGINE_CORE_API Object final : public Abstracts::ObjectBase
	{
		GENERATE_BODY
	public:
		OBJECT_T(DEF_OBJ_T_NONE)
		Object();

	protected:
		Strong<Abstracts::ObjectBase> cloneImpl() const override;

	};
} // namespace Engine
