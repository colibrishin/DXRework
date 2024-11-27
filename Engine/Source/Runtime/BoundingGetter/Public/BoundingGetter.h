#pragma once
#include "Source/Runtime/GenericBounding/Public/GenericBounding.hpp"
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
	namespace Abstracts
	{
		class ObjectBase;
	}

	struct bounding_getter
	{
		static GenericBounding<> value(const Weak<Abstracts::ObjectBase>& object);
	};
}