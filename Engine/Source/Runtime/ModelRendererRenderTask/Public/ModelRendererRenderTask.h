#pragma once
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"

namespace Engine 
{
    struct ModelRendererRenderInstanceTask : public RenderInstanceTask 
    {
        void Run(
            Scene const* scene,
            RenderMap*   render_map,
            const size_t       map_size,
            std::atomic<uint64_t>& instance_count
        ) override;

		void Cleanup(RenderMap* render_map, const size_t map_size) override;
    };
}