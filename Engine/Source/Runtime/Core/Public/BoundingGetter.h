#pragma once
#include "GenericBounding.hpp"
#include "TypeLibrary.h"

namespace Engine 
{
	namespace Abstracts
	{
		class ObjectBase;
	}

	struct ENGINE_CORE_API bounding_getter
	{
		static GenericBounding<> value(const Weak<Abstracts::ObjectBase>& object);
	};
}