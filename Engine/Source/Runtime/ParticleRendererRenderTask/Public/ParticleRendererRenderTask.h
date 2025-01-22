#pragma once
#include "RenderTask.h"

namespace Engine
{
    struct ParticleRendererRenderInstanceTask : public RenderInstanceTask
    {
        void Run(
            Scene const* scene, 
            const RenderMapValueType* render_map,
            std::atomic<uint64_t>& instance_count) override;

		void Cleanup(RenderMapValueType* render_map) override;
    };
}