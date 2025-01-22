#pragma once
#include "Source/Runtime/Core/GenericBounding/Public/GenericBounding.hpp"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
	namespace Abstracts
	{
		class ObjectBase;
	}

	struct CORE_API bounding_getter
	{
		static GenericBounding<> value(const Weak<Abstracts::ObjectBase>& object);
	};
}