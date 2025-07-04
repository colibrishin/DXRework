#pragma once
#include <memory>

#include "TypeLibrary.h"
#include "IGraphicAPI_Extensions.h"

#include "RenderPassTask.generated.h"

namespace Engine
{
    using ContextSetupFunction = std::function<void( const IGraphicContext* )>;

    ECLASS( virtual )
    struct ENGINE_CORE_API RenderPassTask
    {
        GENERATE_BODY
        virtual ~RenderPassTask() = default;
        RenderPassTask()          = default;

        RenderPassTask( const RenderPassTask& ) = delete;
        RenderPassTask& operator=( const RenderPassTask& ) = delete;

        virtual void PreRun( RenderMap const* render_map, const size_t render_map_count, const ObjectPredication& predication ) = 0;
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
} // namespace Engine
