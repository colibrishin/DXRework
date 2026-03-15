#include "AnimatorPrimitive.h"
#include "RenderType.h"

#include "SIMDExtension.hpp"

void Engine::Graphics::AnimatorPrimitive::Apply(SBs::InstanceSB& instance) const
{
    *instance.EvaluateAddress<float>(0) = currentFrame;
    SIMDExtension::_mm256_memcpy(instance.EvaluateAddress<int>(1), &animationID, sizeof(int) * 7);
}
