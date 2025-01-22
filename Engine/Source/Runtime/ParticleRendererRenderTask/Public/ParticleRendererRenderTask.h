#pragma once
#include "RenderTask.h"

namespace Engine
{
    struct ParticleRendererRenderInstanceTask : public RenderInstanceTask
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