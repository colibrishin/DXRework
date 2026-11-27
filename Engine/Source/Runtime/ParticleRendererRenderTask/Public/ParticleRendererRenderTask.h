#pragma once
#include "InstanceSBAllocator.h"
#include "RenderInstanceTask.h"
#include "SingletonSpinLock.h"

#include "ParticleRendererRenderTask.generated.h"

namespace Engine
{
    struct ENGINE_PARTICLERENDERERRENDERTASK_API ParticleRendererRenderInstanceTask : public RenderInstanceTask
    {
        INLINE_COMPILE_TIME_TYPENAME(ParticleRendererRenderInstanceTask)

        ~ParticleRendererRenderInstanceTask() override;
        ParticleRendererRenderInstanceTask();

        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;
        
        // Instances for count pairs, from the per-frame pool.
        void GetInstances(size_t count, aligned_vector<InstancePair>& out);

        // Contiguous instances of one emitter mesh for this frame (InstancePair::instanceCount).
        Graphics::SBs::InstanceSB* GetBlock(size_t count);

        SpinLockTicket m_instance_ticket_;
        SpinLockTicket m_block_ticket_;
        std::vector<std::unique_ptr<aligned_vector<Graphics::SBs::InstanceSB>>> m_blocks_;
        size_t m_blocks_used_{};
        aligned_vector<Graphics::SBs::InstanceSB*> m_instance_generated_;
        size_t m_allocation_count_{};
        size_t m_used_count_{};
    };
}
