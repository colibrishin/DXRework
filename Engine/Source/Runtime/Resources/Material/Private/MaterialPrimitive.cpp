#include "MaterialPrimitive.h"
#include "MaterialPrimitive.generated.h"
#include "StructuredBuffer/Public/StructuredBuffer.h"

#include "SIMDExtension/Public/SIMDExtension.hpp"

void Engine::Graphics::MaterialPrimitive::Apply(SBs::InstanceSB& instance) const
{
    SIMDExtension::_mm256_memcpy(instance.EvaluateAddress<float>(1), this, sizeof(float) * 4);
    SIMDExtension::_mm256_memcpy(instance.EvaluateAddress<Vector4>(0), this, sizeof(Vector4) * 3);
    SIMDExtension::_mm256_memcpy(instance.EvaluateAddress<int>(8), this, sizeof(int) * (g_max_texture_per_material + 3));
}
