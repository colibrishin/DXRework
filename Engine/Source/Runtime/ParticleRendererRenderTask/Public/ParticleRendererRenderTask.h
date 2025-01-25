#pragma once
#include "RenderTask.h"

#include "SingletonSpinLock/Public/SingletonSpinLock.h"
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

        ~ParticleRendererRenderInstanceTask();
        ParticleRendererRenderInstanceTask();

        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;
        
        Graphics::SBs::InstanceSB* GetInstance();

        SpinLockTicket m_instance_ticket_;
        aligned_vector<Graphics::SBs::InstanceSB*> m_instance_generated_;
        u_fast_pool_allocator_single<Graphics::SBs::InstanceSB> m_instance_allocator_;
        size_t m_allocation_count_{};
        size_t m_used_count_{};
    };
}
