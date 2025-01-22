#pragma once
#include "Serialization.hpp"
#include "AnimatorPrimitive.generated.h"

namespace Engine::Graphics
{
	ECLASS(serialize)
	struct ENGINE_ANIMATOR_API AnimatorPrimitive
	{
		GENERATE_BODY

		EPROPERTY()
		float currentFrame;

		EPROPERTY()
		int animationID;
		EPROPERTY()
		int animationDuration;
		EPROPERTY()
		int noAnimation;
		EPROPERTY()
		int atlasX;
		EPROPERTY()
		int atlasY;
		EPROPERTY()
		int atlasW;
		EPROPERTY()
		int atlasH;
	};
}
