#pragma once
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"

namespace Engine
{
	// Anonymous non-type defined object
	class CORE_API Object final : public Abstracts::ObjectBase
	{
	public:
		OBJECT_T(DEF_OBJ_T_NONE)
		Object();

	protected:
		Strong<Abstracts::ObjectBase> cloneImpl() const override;

	private:
		SERIALIZE_DECL
	};
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Object)
