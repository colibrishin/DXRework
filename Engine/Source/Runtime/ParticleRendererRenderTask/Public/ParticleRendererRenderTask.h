#pragma once
#include "RenderTask.h"

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

namespace Engine
{
    struct ENGINE_PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTaskModule : public IModule
    {
        INLINE_COMPILE_TIME_TYPENAME(ParticleRendererRenderInstanceTaskModule)
	    void Initialize() override;
	    void Shutdown() override;
	    bool DynamicLoadable() override;
    };

    struct ENGINE_PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTask : public RenderInstanceTask
    {
        INLINE_COMPILE_TIME_TYPENAME(ParticleRendererRenderInstanceTask)
        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size,
            std::atomic<uint64_t>& instance_count
        ) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;
    };
}
