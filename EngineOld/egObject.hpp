#pragma once
#include "egObjectBase.hpp"

namespace Engine
{
	// Anonymous non-type defined object
	class Object final : public Abstract::ObjectBase
	{
	public:
		OBJECT_T(DEF_OBJ_T_NONE)
		Object();

	protected:
		StrongObjectBase cloneImpl() const override;

	private:
		SERIALIZE_DECL
	};
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Object)
