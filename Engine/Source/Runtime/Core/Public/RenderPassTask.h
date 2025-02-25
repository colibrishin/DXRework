#pragma once
#include <memory>

#include "GraphicInterface.h"
#include "TypeLibrary.h"

#include "RenderPassTask.generated.h"

namespace Engine
{
    using ContextSetupFunction = std::function<void( const GraphicInterfaceContextPrimitive* )>;

    ECLASS( virtual )
    struct ENGINE_CORE_API RenderPassTask
    {
        GENERATE_BODY
        virtual ~RenderPassTask() = default;
        virtual void
        PreRun( RenderMap const* render_map, const size_t render_map_count, const ObjectPredication& predication )  = 0;
        virtual void Run( float                                                             dt,
                          bool                                                              shader_bypass,
                          RenderMap const*                                                  domain_map,
                          const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
                          const Graphics::SBs::LocalParamSB&                                local_param,
                          const ObjectPredication&                                          predicate,
                          const ContextSetupFunction&                                       prerender_predicate,
                          const ContextSetupFunction&                                       postrender_predicate,
                          const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
                          const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates ) = 0;

        virtual void Cleanup() = 0;
    };

    struct ENGINE_CORE_API IRenderPassTaskFactory
    {
        virtual ~IRenderPassTaskFactory()                       = default;
        virtual RenderPassTask* New()                           = 0;
        virtual void            Release( RenderPassTask* task ) = 0;
        virtual HashType        GetTaskType() const             = 0;
    };

    template <typename T> requires std::is_base_of_v<RenderPassTask, T>
    struct RenderPassTaskFactory : IRenderPassTaskFactory
    {
        ~RenderPassTaskFactory() override = default;

        RenderPassTaskFactory()
        {
            m_values_.reserve( 1 << 8 );
        }

        RenderPassTask* New() override
        {
            T& value = m_values_.emplace_back( T{} );
            return &value;
        }

        void Release(RenderPassTask* task) override
        {
            std::erase_if( m_values_, [ &task ]( const T& elem ) { return &elem == task; } );
        }

        HashType GetTaskType() const override
        {
            return T::StaticTypeHash();
        }

    private:
        std::vector<T, u_pool_allocator_single<T>> m_values_{};
    };

} // namespace Engine
