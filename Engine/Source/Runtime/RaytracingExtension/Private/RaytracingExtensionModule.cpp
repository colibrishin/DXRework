#if CFG_RAYTRACING
#include "RaytracingExtensionModule.h"

#include <cstdlib>

#include "RaytracingExtension.h"
#include "TaskScheduler.h"

MODULE_IMPL( Engine::RaytracingExtensionModule, RaytracingExtension )

namespace Engine
{
    void RaytracingProxyFunc(const std::vector<std::any>& param, const float)
    {
        if (param[0].has_value())
        {
            const bool toggle = std::any_cast<bool>( param[0] );
            RaytracingExtension::SetRaytracing( toggle );
        }
    }

    bool RaytracingExtensionModule::InitializeImpl()
	{
	    Managers::TaskScheduler::GetInstance().Inject( TASK_TOGGLE_RASTER, RaytracingProxyFunc );

	    // KOLIBRI_RAYTRACING=1 starts with ray tracing on (off by default; the editor checkbox still toggles it).
	    if ( const char* v = std::getenv( "KOLIBRI_RAYTRACING" ); v && v[ 0 ] == '1' )
	    {
	        Managers::TaskScheduler::GetInstance().AddTask( TASK_TOGGLE_RASTER, { true, 0.f } );
	    }
	    return true;
	}

	bool RaytracingExtensionModule::ShutdownImpl()
	{
	    Managers::TaskScheduler::GetInstance().Extract( TASK_TOGGLE_RASTER, RaytracingProxyFunc );
	    return true;
	}

	bool RaytracingExtensionModule::DynamicLoadable()
	{
		return true;
	}

}
#endif