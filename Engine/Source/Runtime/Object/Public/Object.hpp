#pragma once
#include <boost/serialization/export.hpp>

#include "Source/Runtime/Abstracts/CoreObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Serialization/Public/SerializationHelper.hpp"

namespace Engine
{
	// Anonymous non-type defined object
	class Object final : public Abstracts::ObjectBase
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