#pragma once
#include <atomic>
#include <ranges>
#include <mutex>

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "RenderTask.h"
#include "Delegation/Public/Delegation.hpp"

#include "Renderer.generated.h"

DEFINE_DELEGATE(OnRenderDone, const Engine::eShaderDomain);
DEFINE_DELEGATE(OnRenderTaskDirty);

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_RENDERPIPELINE_API Renderer : public Abstracts::Singleton<Renderer>
	{
		GENERATE_BODY
	public:
		explicit Renderer(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_b_ready_(false) {}

		DelegateOnRenderDone onRenderDone;
        DelegateOnRenderTaskDirty onRenderTaskDirty;
		
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Initialize() override;
		
		void RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task);
        void RegisterRenderPass( const std::wstring_view name, RenderPassTask *task );
        void RenderPassWith( const std::wstring_view name, const eShaderDomain domain );

        void UnregisterRenderInstance( const std::wstring_view name );
        void UnregisterRenderPass( const std::wstring_view name );
        void RenderPassWithout( const std::wstring_view name, const eShaderDomain domain );

		void RegisterStructuredBuffer(const StructuredBufferDecorator* sb);
		void UnregisterStructuredBuffer(const StructuredBufferDecorator* sb);

		void RegisterContextPreRenderSetup(const std::string_view name, const ContextSetupFunction& prerender_func);
		void UnregisterContextPreRenderSetup(const std::string_view name);
		void RegisterContextPostRenderSetup(const std::string_view name, const ContextSetupFunction& postrender_func);
		void UnregisterContextPostRenderSetup(const std::string_view name);

		using RenderPassTaskUniqueContainer = std::unordered_map<std::wstring, Unique<RenderPassTask>>;
		using RenderPassTaskBorrowedContainer = std::unordered_map<std::wstring, RenderPassTask*>;

        template <typename Cont>
        void RenderPass( 
                const Cont                                                        container[ SHADER_DOMAIN_MAX ],
                float                                                             dt,
                bool                                                              shader_bypass,
                eShaderDomain                                                     domain,
                const SBs::LocalParamSB                                          &local_param_sb,
                const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                const ObjectPredication                                          &predication,
                const ContextSetupFunction                                       &prerender_predicate,
                const ContextSetupFunction                                       &postrender_predicate,
                const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_funcs,
                const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_funcs ) const
        {
            if constexpr ( std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype( m_render_pass_tasks_[ 0 ] )>>, Cont> )
            {
                for ( const auto &task : container[ domain ] | std::views::values )
                {
                    task->Run( dt,
                               shader_bypass,
                               &m_render_candidates_[ domain ],
                               additional_sbs,
                               local_param_sb,
                               predication,
                               prerender_predicate,
                               postrender_predicate,
                               prerender_funcs,
                               postrender_funcs );
                }
            }
            else
            {
                for ( const auto &task : container[ domain ] )
                {
                    task->Run( dt,
                               shader_bypass,
                               &m_render_candidates_[ domain ],
                               additional_sbs,
                               local_param_sb,
                               predication,
                               prerender_predicate,
                               postrender_predicate,
                               prerender_funcs,
                               postrender_funcs );
                }
            }
        }

        template <typename Cont>
        void RenderPassAssisted( const Cont               container[ SHADER_DOMAIN_MAX ],
                                 float                    dt,
                                 bool                     shader_bypass,
                                 eShaderDomain            domain,
                                 const SBs::LocalParamSB &local_param_sb,
                                 const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                 const ObjectPredication                                 &predication,
                                 const ContextSetupFunction                              &prerender_predicate,
                                 const ContextSetupFunction                              &postrender_predicate ) const
        {
            RenderPass( container[ SHADER_DOMAIN_MAX ],
                        dt,
                        shader_bypass,
                        domain,
                        local_param_sb,
                        additional_sbs,
                        predication,
                        prerender_predicate,
                        postrender_predicate,
                        m_prerender_funcs_,
                        m_postrender_funcs_ );
        }

		template <typename... ExcludeRenderTaskTs>
		struct ExclusionPredicate
		{
            static void ResolveDirtyness( const RenderPassTaskBorrowedContainer cont[ SHADER_DOMAIN_MAX ],
                                          std::vector<RenderPassTask *> tasks[ SHADER_DOMAIN_MAX ] )
			{
                for ( size_t i = 0; i < SHADER_DOMAIN_MAX; ++i )
                {
                    tasks[ i ]->clear();
                    tasks[ i ]->reserve( cont[ i ]->size() );

                    for ( const auto &task : cont[ i ] | std::views::values )
                    {
                        if ( bool check[] = { task->GetTypeHash() == ExcludeRenderTaskTs::StaticTypeHash()... };
                             std::any_of( std::begin( check ),
                                          std::end( check ),
                                          []( const bool b ) { return b == true; } ) )
                        {
                            continue;
                        }

                        if ( task != nullptr )
                        {
                            tasks[ i ]->emplace_back( task.get() );
                        }
                    }
                }
			}
		};

		template <typename... IncludeRenderTaskTs>
        struct InclusionPredicate
        {
            static void ResolveDirtyness( const RenderPassTaskBorrowedContainer cont[ SHADER_DOMAIN_MAX ],
                                          std::vector<RenderPassTask *> tasks[ SHADER_DOMAIN_MAX ] )
            {
                for ( size_t i = 0; i < SHADER_DOMAIN_MAX; ++i )
                {
                    tasks[ i ]->clear();
                    tasks[ i ]->reserve( cont[ i ]->size() );

                    for ( const auto &task : cont[ i ] | std::views::values )
                    {
                        if ( bool check[] = { task->GetTypeHash() == IncludeRenderTaskTs::StaticTypeHash()... };
                             std::any_of( std::begin( check ),
                                          std::end( check ),
                                          []( const bool b ) { return b == true; } ) )
                        {
                            tasks[ i ]->emplace_back( task.get() );
                        }
                    }
                }
            }
        };

	    template <typename PredicationT>
        void RenderPassVanillaPredicate(
            float                                                             dt,
            bool                                                              shader_bypass,
            eShaderDomain                                                     domain,
            const Graphics::SBs::LocalParamSB&                                local_param_sb,
            const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
            const ObjectPredication&                                          predication,
            const ContextSetupFunction&                                       prerender_predicate,
            const ContextSetupFunction&                                       postrender_predicate,
            const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_funcs,
            const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_funcs
        )
	    {
	        static std::vector<RenderPassTask*> render_pass_task_copy[ SHADER_DOMAIN_MAX ];
            static std::once_flag               delegate_flag;
            static const auto& func = [ this ]()
            {
                PredicationT::ResolveDirtyness( m_render_pass_tasks_, render_pass_task_copy );
            };

            std::call_once( delegate_flag, [ this ]()
            {
                func();
                onRenderTaskDirty.Listen( func );
            } );

            RenderPass( render_pass_task_copy,
                        dt,
                        shader_bypass,
                        domain,
                        local_param_sb,
                        additional_sbs,
                        predication,
                        prerender_predicate,
                        postrender_predicate,
                        prerender_funcs,
                        postrender_funcs );
	    }

	    template <typename PredicationT>
        void RenderPassAssistedPredicate(
            float                                                   dt,
            bool                                                    shader_bypass,
            eShaderDomain                                           domain,
            const Graphics::SBs::LocalParamSB&                      local_param_sb,
            const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
            const ObjectPredication&                                predication,
            const ContextSetupFunction&                             prerender_predicate,
            ContextSetupFunction&                             postrender_predicate
        )
	    {
            static std::vector<RenderPassTask *> render_pass_task_copy[ SHADER_DOMAIN_MAX ];
            static std::once_flag               delegate_flag;
            static const auto& func = [ this ]()
            {
                PredicationT::ResolveDirtyness( m_render_pass_tasks_, render_pass_task_copy );
            };

            std::call_once( delegate_flag, [ this ]()
            {
                func();
                onRenderTaskDirty.Listen( func );
            } );

            RenderPassAssisted( render_pass_task_copy,
                                dt,
                                shader_bypass,
                                domain,
                                local_param_sb,
                                additional_sbs,
                                predication,
                                prerender_predicate,
                                postrender_predicate );
	    }

		template <typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void
        RenderPassVanillaExclusion(
			    float                                                    dt,
				bool                                                     shader_bypass,
				eShaderDomain                                            domain,
				const Graphics::SBs::LocalParamSB& local_param_sb,
				const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
				const ObjectPredication& predication,
				const ContextSetupFunction& prerender_predicate,
				const ContextSetupFunction& postrender_predicate,
				const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_funcs,
				const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_funcs)
		{
            RenderPassVanillaPredicate<ExclusionPredicate<RenderTaskTs...>>( dt,
                                                                                shader_bypass,
                                                                                domain,
                                                                                local_param_sb,
                                                                                additional_sbs,
                                                                                predication,
                                                                                prerender_predicate,
                                                                                postrender_predicate,
                                                                                prerender_funcs,
                                                                                postrender_funcs );
		}

		template <typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void
        RenderPassVanillaInclusion( float                                                    dt,
                                   bool                                                     shader_bypass,
                                   eShaderDomain                                            domain,
                                   const Graphics::SBs::LocalParamSB                       &local_param_sb,
                                   const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                   const ObjectPredication                                 &predication,
                                   const ContextSetupFunction                              &prerender_predicate,
                                   const ContextSetupFunction                              &postrender_predicate,
                                   const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_funcs,
                                   const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_funcs )
        {
            RenderPassVanillaPredicate<InclusionPredicate<RenderTaskTs...>>( dt,
                                                                                shader_bypass,
                                                                                domain,
                                                                                local_param_sb,
                                                                                additional_sbs,
                                                                                predication,
                                                                                prerender_predicate,
                                                                                postrender_predicate,
                                                                                prerender_funcs,
                                                                                postrender_funcs );
        }

		template <typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
		void RenderPassAssistedInclusion(
			float                                                    dt,
			bool                                                     shader_bypass,
			eShaderDomain                                            domain,
			const Graphics::SBs::LocalParamSB& local_param_sb,
			const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
			const ObjectPredication& predication,
			const ContextSetupFunction& prerender_predicate,
			const ContextSetupFunction& postrender_predicate) const
		{
            RenderPassAssistedPredicate<InclusionPredicate<RenderTaskTs...>>( dt,
                                                                           shader_bypass,
                                                                           domain,
                                                                           local_param_sb,
                                                                           additional_sbs,
                                                                           predication,
                                                                           prerender_predicate,
                                                                           postrender_predicate,
                                                                           m_prerender_funcs_,
                                                                           m_postrender_funcs_ );
		}

		template <typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void RenderPassAssistedExclusion( float                                                    dt,
                                          bool                                                     shader_bypass,
                                          eShaderDomain                                            domain,
                                          const Graphics::SBs::LocalParamSB                       &local_param_sb,
                                          const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                          const ObjectPredication                                 &predication,
                                          const ContextSetupFunction                              &prerender_predicate,
                                          const ContextSetupFunction &postrender_predicate ) const
        {
            RenderPassAssistedPredicate<ExclusionPredicate<RenderTaskTs...>>( dt,
                                                                           shader_bypass,
                                                                           domain,
                                                                           local_param_sb,
                                                                           additional_sbs,
                                                                           predication,
                                                                           prerender_predicate,
                                                                           postrender_predicate,
                                                                           m_prerender_funcs_,
                                                                           m_postrender_funcs_ );
        }

		[[nodiscard]] bool Ready() const;

	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		~Renderer() override;
		
		bool m_b_ready_ = false;
		std::unordered_map<std::string_view, ContextSetupFunction> m_prerender_funcs_;
		std::unordered_map<std::string_view, ContextSetupFunction> m_postrender_funcs_;
		aligned_vector<const StructuredBufferDecorator*> m_additional_sbs_;

	    bool m_b_task_dirty_ = false;
	    
		std::unordered_map<std::wstring, Unique<RenderInstanceTask>> m_render_instance_tasks_;
        RenderPassTaskUniqueContainer                                m_unique_render_pass_tasks_;
        RenderPassTaskBorrowedContainer                              m_render_pass_tasks_[ SHADER_DOMAIN_MAX ];
        
		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
