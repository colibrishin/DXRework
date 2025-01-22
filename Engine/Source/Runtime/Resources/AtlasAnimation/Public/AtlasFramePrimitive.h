#pragma once
#include <filesystem>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Serialization.hpp"
#include "AtlasFramePrimitive.generated.h"

namespace Engine
{
	ECLASS()
	struct ENGINE_ATLASANIMATION_API AtlasFramePrimitive
	{
		GENERATE_BODY

		AtlasFramePrimitive() = default;
		AtlasFramePrimitive(UINT x, UINT y, UINT width, UINT height, float duration) :
			X(x), Y(y), Width(width), Height(height), Duration(duration) {}

		EPROPERTY()
		UINT  X = 0;
		EPROPERTY()
		UINT  Y = 0;
		EPROPERTY()
		UINT  Width = 0;
		EPROPERTY()
		UINT  Height = 0;
		EPROPERTY()
		float Duration = 0.f;
	};
}