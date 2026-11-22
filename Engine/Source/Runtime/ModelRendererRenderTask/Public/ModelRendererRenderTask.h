#pragma once
#include "IModule.h"
#include "InstanceSBAllocator.h"
#include "RenderInstanceTask.h"
#include "Scene.h"

#include "ModelRendererRenderTask.generated.h"

namespace Engine
{
    ECLASS(module)
    struct ModelRendererRenderInstanceTaskModule : public IModule
    {
        GENERATE_BODY
        bool InitializeImpl() override;
        bool ShutdownImpl() override;
	    bool DynamicLoadable() override;
    };

    struct ModelRendererRenderInstanceTask : public RenderInstanceTask 
    {
        ModelRendererRenderInstanceTask();
        ~ModelRendererRenderInstanceTask();

        INLINE_COMPILE_TIME_TYPENAME(ModelRendererRenderInstanceTask)
        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;

        // Takes count instance buffers under one lock (a thread keeps them as spares).
        void GetInstances( size_t count, aligned_vector<Graphics::SBs::InstanceSB*>& out );

    private:
        Graphics::SBs::InstanceSB* GetInstanceLocked(); // under m_instance_ticket_

    public:
        SpinLockTicket m_instance_ticket_;
        aligned_vector<Graphics::SBs::InstanceSB*> m_instance_generated_;
        size_t m_allocation_count_{};
        size_t m_used_count_{};
    };
}