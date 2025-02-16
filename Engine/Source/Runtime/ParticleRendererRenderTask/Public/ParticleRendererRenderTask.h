#pragma once
#include "RenderTask.h"

#include "SingletonSpinLock/Public/SingletonSpinLock.h"

#include "ParticleRendererRenderTask.generated.h"

namespace Engine
{
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
