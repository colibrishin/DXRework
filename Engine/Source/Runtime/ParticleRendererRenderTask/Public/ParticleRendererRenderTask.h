#pragma once
#include "RenderTask.h"

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

namespace Engine
{
    
    struct PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTaskModule : public IModule
    {
	    void Initialize() override;
	    void Shutdown() override;
	    bool DynamicLoadable() override;
    };

    struct PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTask : public RenderInstanceTask
    {
        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size,
            std::atomic<uint64_t>& instance_count
        ) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;
    };
}
