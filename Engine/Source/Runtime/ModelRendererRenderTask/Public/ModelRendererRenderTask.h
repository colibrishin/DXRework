#pragma once
#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"

namespace Engine
{
    struct ModelRendererRenderInstanceTaskModule : public IModule
    {
        INLINE_COMPILE_TIME_TYPENAME(ModelRendererRenderInstanceTaskModule)
	    void Initialize() override;
	    void Shutdown() override;
	    bool DynamicLoadable() override;
    };

    struct ModelRendererRenderInstanceTask : public RenderInstanceTask 
    {
        ModelRendererRenderInstanceTask();

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
    };
}