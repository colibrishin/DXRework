#include "RaytracingRenderPassTaskModule.h"
#include "RaytracingRenderPassTaskModule.generated.h"

#include "ModuleManager.h"
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
	    Managers::Renderer::GetInstance().RegisterRenderPass( type_name_wstr, new RaytracingRenderPassTask() );
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
