#pragma once
#include <memory>

#include "ConcurrentTypeLibrary.h"
#include "IGraphicAPI.h"
#include "RenderPassTask.h"
#include "RenderPassTaskFactory.h"
#include "SingletonSpinLock.h"
#include "Texture.h"
#include "TexturePair.h"
#include "TypeLibrary.h"

#include "ForwardRenderPassTask.generated.h"

namespace Engine
{
	ECLASS(virtual)
    struct ENGINE_FORWARDRENDERPASSTASK_API ForwardRenderPassTask : RenderPassTask
	{
		GENERATE_BODY
        ForwardRenderPassTask();

        ForwardRenderPassTask( ForwardRenderPassTask&& other ) noexcept
            : m_gi_ticket_( std::move( other.m_gi_ticket_ ) ),
              m_local_param_pool_ticket_( std::move( other.m_local_param_pool_ticket_ ) ),
              m_instance_pool_ticket_( std::move( other.m_instance_pool_ticket_ ) ),
              m_texture_record_ticket_( std::move( other.m_texture_record_ticket_ ) )
        {
            m_local_param_pool_     = std::move( other.m_local_param_pool_ );
            m_instance_pool_        = std::move( other.m_instance_pool_ );
            m_heaps_                = std::move( m_heaps_ );
            m_used_shader_textures_ = std::move( m_used_shader_textures_ );
        }

		ForwardRenderPassTask& operator=(ForwardRenderPassTask&& other) noexcept
		{
		    m_gi_ticket_               = std::move( other.m_gi_ticket_ );
            m_local_param_pool_ticket_ = std::move( other.m_local_param_pool_ticket_ );
            m_instance_pool_ticket_    = std::move( m_instance_pool_ticket_ );
            m_texture_record_ticket_   = std::move( m_texture_record_ticket_ );

            m_local_param_pool_     = std::move( other.m_local_param_pool_ );
            m_instance_pool_        = std::move( other.m_instance_pool_ );
            m_heaps_                = std::move( m_heaps_ );
            m_used_shader_textures_ = std::move( m_used_shader_textures_ );

			return *this;
		}

		void Run( float                                                             dt,
                  bool                                                              shader_bypass,
                  RenderMap const*                                                  domain_map,
                  const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
                  const Graphics::SBs::LocalParamSB&                                local_param,
                  const ObjectPredication&                                          predicate,
                  const ContextSetupFunction&                                       prerender_predicate,
                  const ContextSetupFunction&                                       postrender_predicate,
                  const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
                  const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates ) override;
		
		void Cleanup() override;

	private:
		using IntermediateShaderMap = concurrent_fast_pool_map<Resources::ShaderBase*, aligned_vector<InstancePair>>;

		inline void PredicateObject( const ObjectPredication &predicate,
                                     RenderMap const         *domain_map,
                                     uint64_t                &instance_count,
                                     IntermediateShaderMap   &out_map ) const;

		inline void StartPhase_MultiThread( float                                                             dt,
                                            bool                                                              shader_bypass,
                                            const Resources::Shader                                          *shader,
                                            const Resources::Mesh                                            *mesh,
                                            const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                                            const Graphics::SBs::LocalParamSB                                &local_param,
                                            const ContextSetupFunction                                       &prerender_predicate,
                                            const ContextSetupFunction                                       &postrender_predicate,
                                            const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
                                            const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates,
                                            const aligned_vector<InstancePair>                               &instance_pairs );

		void RecordUsedTexture( const IGraphicContext* context,
                                IGraphicAPI&                       gi,
                                const Resources::Texture*               tex );


		inline void DrawPhase_MultiThread( float                                                 dt,
                                           bool                                                  shader_bypass,
                                           size_t                                                instance_count,
                                           StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance_buffer,
                                           const Resources::Shader                              *shader,
                                           const Resources::Mesh                                *mesh,
                                           const IGraphicContext               *context,
                                           const aligned_vector<Graphics::SBs::InstanceSB *>    &instances,
                                           const aligned_vector<TexturePair>                    &texture_pairs );

    public:
        void PreRun( const RenderMap*         render_map,
                     const size_t             render_map_count,
                     const ObjectPredication& predication ) override;

    private:
        SpinLockTicket m_gi_ticket_;
		SpinLockTicket m_local_param_pool_ticket_;
		SpinLockTicket m_instance_pool_ticket_;
		SpinLockTicket m_texture_record_ticket_;

		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_{};
		tbb::concurrent_vector<Unique<IHeapBase>> m_heaps_{};
		std::vector<const Resources::Texture*> m_used_shader_textures_{};
	};

    template struct ENGINE_FORWARDRENDERPASSTASK_API RenderPassTaskFactory<ForwardRenderPassTask>;
}
