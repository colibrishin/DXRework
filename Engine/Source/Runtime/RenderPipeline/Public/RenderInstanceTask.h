#pragma once
#include <memory>

#include "IGraphicAPI_Extensions.h"
#include "RenderType.h"
#include "TypeLibrary.h"

#include "RenderInstanceTask.generated.h"

namespace Engine
{
    ECLASS( virtual )
    struct ENGINE_RENDERPIPELINE_API RenderInstanceTask
    {
        GENERATE_BODY
        virtual      ~RenderInstanceTask() = default;
        virtual void Run(Scene const* scene, RenderMap* render_map, const size_t map_size) = 0;
        virtual void Cleanup(RenderMap* render_map, const size_t map_size) = 0;
        // Remove everything the task put into render_map: called when there is no active scene to draw and before the
        // task is unregistered.
        virtual void Release(RenderMap* render_map, const size_t map_size) { Cleanup( render_map, map_size ); }
    };
}
