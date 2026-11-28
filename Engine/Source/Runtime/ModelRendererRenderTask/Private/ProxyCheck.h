#pragma once
#include <cstddef>

#include "RenderType.h"

namespace Engine
{
    class Scene;
}

namespace Engine::ProxyCheck
{
    // Render-list check, Editor and Debug builds only (KOLIBRI_PROXY_CHECK=1). After the ModelRenderer instances of a
    // frame are in render_map, builds them again from the scene the per-frame way (freshly reset buffers, inactive objects
    // left out) into a scratch map and ends the process when the two differ: per (domain, shader, mesh) the multiset
    // of (object, instance values, textures), and which domains hold the ModelRenderer key. The texture slot fields
    // the passes write are left out. KOLIBRI_PROXY_MUTATE=world changes one instance first, so the check is seen to
    // fail. Runs on the main thread at the end of the instance task's Run.
    void Run( const Scene& scene, RenderMap* render_map, size_t map_size );
}
