#pragma once
#include "ModuleManager.h"

#include "RenderTask.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"

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

        Graphics::SBs::InstanceSB* GetInstance();

        SpinLockTicket m_instance_ticket_;
        aligned_vector<Graphics::SBs::InstanceSB*> m_instance_generated_;
        u_fast_pool_allocator_single<Graphics::SBs::InstanceSB> m_instance_allocator_;
        size_t m_allocation_count_{};
        size_t m_used_count_{};
    };
}