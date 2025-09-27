#pragma once
#include <memory>

#include "IGraphicAPI_Extensions.h"
#include "TypeLibrary.h"

#include "RenderInstanceTask.generated.h"

namespace Engine 
{
    ECLASS( virtual )
	struct ENGINE_CORE_API RenderInstanceTask
	{
		GENERATE_BODY
		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, RenderMap* render_map, const size_t map_size) = 0;
		virtual void Cleanup(RenderMap* render_map, const size_t map_size) = 0;
	};
}
