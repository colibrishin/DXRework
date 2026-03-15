#include "MaterialPrimitive.h"
#include "StructuredBuffer.h"

#include "SIMDExtension.hpp"

void Engine::Graphics::MaterialPrimitive::Apply( SBs::InstanceSB& instance ) const
{
    SIMDExtension::_mm256_memcpy( instance.EvaluateAddress<float>( 1 ), this, sizeof( float ) * 4 );
    SIMDExtension::_mm256_memcpy( instance.EvaluateAddress<Vector4>( 0 ), &overrideColor, sizeof( Vector4 ) * 3 );
    SIMDExtension::_mm256_memcpy( instance.EvaluateAddress<int>( 8 ),
                                  &repeatTexture,
                                  sizeof( int ) * ( g_max_texture_per_material + g_max_texture_per_material +
                                                    g_max_texture_per_material + 2 ) );
}
