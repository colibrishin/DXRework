#pragma once

#if CFG_RAYTRACING

#include "IGraphicAPI.h"
#include "RenderType.h"
#include "TypeLibrary.h"

namespace Engine
{
namespace Resources { class RaytracingShader; }

    struct ENGINE_RENDERPIPELINE_API IRaytracingExtension : public virtual IGraphicAPIBase
    {
        ~IRaytracingExtension() override = default;
        INLINE_COMPILE_TIME_TYPENAME( IRaytracingExtension )

        virtual bool IsRaytracingSupported() = 0;
        virtual void InitializeRaytracing()  = 0;
        virtual void ShutdownRaytracing()    = 0;

        void UseRaytracing( const bool flag )
        {
            if ( IsRaytracingSupported() )
            {
                m_b_raytracing_ = flag;
            }
        }
        [[nodiscard]] bool ShouldUseRaytracing() const noexcept
        {
            return m_b_raytracing_;
        }

        virtual Unique<IHeapBase>  GetRaytracingHeap()      = 0;
        virtual IRaytracingShader* GetNewRaytracingShader() = 0;

        virtual void* GetRaytracingNativeInterface() = 0;
        virtual void* GetRaytracingNativePipeline()  = 0;

        virtual bool BuildTopLevelAccelerationBuffer( const IGraphicContext*   context,
                                                      RenderMap const*         render_map,
                                                      size_t                   render_map_size,
                                                      AccelStructBuffer&       out_tlas_buffer,
                                                      const ObjectPredication& predication = {} ) = 0;

        virtual void DispatchRay( const IGraphicContext*                                       context,
                                  const Resources::RaytracingShader*                           shader,
                                  const StructuredBufferTypeProxy<Graphics::SBs::LightSB>&     light,
                                  const StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>&  instances,
                                  const ConstantBufferTypeProxy<Graphics::CBs::PerspectiveCB>& perspective,
                                  const ConstantBufferTypeProxy<Graphics::CBs::ParamCB>&       param,
                                  const byte_stream&                                           hit_records,
                                  const AccelStructBuffer& top_level_accel_buffer ) = 0;

        virtual void CopyRaytracingToRenderTarget( const IGraphicContext* context ) = 0;

    private:
        bool m_b_raytracing_ = false;
    };
}

#endif
