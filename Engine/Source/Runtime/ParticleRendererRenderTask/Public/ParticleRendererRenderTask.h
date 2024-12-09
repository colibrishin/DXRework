#pragma once
#include "Source/Runtime/Managers/Renderer/Public/RenderTask.h"

namespace Engine
{
    struct ParticleRendererRenderInstanceTask : public RenderInstanceTask
    {
        void Run(
            const Scene const* scene, 
            const RenderMapValueType* render_map, 
            const std::size_t map_size, 
            std::atomic<uint64_t>& instance_count) override;

		virtual void Cleanup(const RenderMapValueType* render_map) override;
    }
}