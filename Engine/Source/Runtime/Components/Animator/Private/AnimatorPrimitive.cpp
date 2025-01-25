#include "AnimatorPrimitive.h"
#include "AnimatorPrimitive.generated.h"
#include "RenderType.h"

#include "SIMDExtension/Public/SIMDExtension.hpp"

void Engine::Graphics::AnimatorPrimitive::Apply(SBs::InstanceSB& instance) const
{
    *instance.EvaluateAddress<float>(0) = currentFrame;
    SIMDExtension::_mm256_memcpy(instance.EvaluateAddress<int>(1), &animationID, sizeof(int) * 7);
}
