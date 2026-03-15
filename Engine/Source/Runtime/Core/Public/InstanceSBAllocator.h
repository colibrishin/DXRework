#pragma once
#include "Allocator.h"
#include "StructuredBuffer.h"

namespace Engine
{
    /** Shared engine pool allocator for Graphics::SBs::InstanceSB. Use for render instance tasks. */
    inline u_fast_pool_allocator_single<Graphics::SBs::InstanceSB>& get_instance_sb_pool_allocator()
    {
        static u_fast_pool_allocator_single<Graphics::SBs::InstanceSB> s_alloc;
        return s_alloc;
    }
}
