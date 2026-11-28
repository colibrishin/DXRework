#pragma once
#include <cstddef>
#include <functional>

#include "InstanceSBAllocator.h"
#include "RenderType.h"

namespace Engine
{
    class Scene;
    namespace Abstracts
    {
        class ObjectBase;
    }

    // The object and all its parents are active.
    bool VisibleInHierarchy( const Abstracts::ObjectBase& object );

    struct BuildOptions
    {
        // Hands out `count` instance buffers (called from worker threads; must lock itself).
        std::function<void( size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out )> acquire;
        // Reset each buffer to a newly allocated one before writing (the reused pool keeps the previous user's
        // unwritten fields otherwise).
        bool reset_instances = false;
        // Leave out objects that are inactive or have an inactive parent.
        bool skip_hidden = false;
    };

    // Builds an instance for every ModelRenderer of the scene into render_map: the per-frame path, also used as the
    // reference of the proxy check. Spare buffers left over are reclaimed by the caller's pool reset.
    void BuildAllInstances( const Scene& scene, RenderMap* render_map, size_t map_size, const BuildOptions& options );
}
