#include "RaytracingRenderPassTaskModule.h"

#include "ModuleManager.h"
#include "ModuleRegistration.h"
#include "RaytracingRenderPassTask.h"
#include "Renderer.h"

MODULE_IMPL( Engine::RaytracingRenderPassTaskModule, RaytracingRenderPassTask )

namespace Engine
{
	bool RaytracingRenderPassTaskModule::InitializeImpl()
	{
	    const std::string_view& type_name = RaytracingRenderPassTask::StaticTypeName();
        const std::wstring type_name_wstr( type_name.begin(), type_name.end() );
        
	    
#if CFG_RAYTRACING
        Managers::Renderer::GetInstance().RegisterRenderPass( type_name_wstr,
                                                              new RenderPassTaskFactory<RaytracingRenderPassTask>() ENGINE_MODULE_SCOPE );
#endif
	    return true;
	}

	bool RaytracingRenderPassTaskModule::ShutdownImpl()
	{
	    const std::string_view& type_name = RaytracingRenderPassTask::StaticTypeName();
	    const std::wstring type_name_wstr( type_name.begin(), type_name.end() );
	    
#if CFG_RAYTRACING
		Managers::Renderer::GetInstance().UnregisterRenderPass( type_name_wstr );
#endif
	    return true;
	}

	bool RaytracingRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}
