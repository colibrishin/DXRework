#pragma once

namespace Engine
{
    class Scene;
}

namespace Engine::DirtyCheck
{
    // Missing-signal check, Editor and Debug builds only (KOLIBRI_DIRTY_CHECK=1). Compares every ModelRenderer input
    // of the active scene with the previous frame and ends the process when one changed without a change signal.
    // KOLIBRI_DIRTY_SCENARIO=<names|all> changes the scene through each path every few frames, and
    // KOLIBRI_DIRTY_MUTATE=<site> drops one call site's events, so the check is seen to fail.
    // Runs on the main thread before the render instances are built.
    void Run( const Scene& scene );
}
