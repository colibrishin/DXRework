#pragma once
#include <atomic>
#include <ranges>
#include <mutex>

#include "Allocator.h"
#include "ConcurrentTypeLibrary.h"
#include "Singleton.h"
#include "RenderType.h"
#include "RenderPassTask.h"
#include "RenderPassTaskFactory.h"
#include "RenderInstanceTask.h"
#include "Delegation.hpp"

#include "Renderer.generated.h"

DEFINE_DELEGATE(OnRenderDone, const Engine::eShaderDomain);
DEFINE_DELEGATE(OnRenderTaskDirty);

#if IS_DLL
namespace Engine
{
    struct RenderTaskTraits
    {
        const std::type_info& rtti;
        const eShaderDomain   domain;

        bool operator==(const RenderTaskTraits& other) const noexcept
        {
            return rtti.name() == other.rtti.name() && domain == other.domain;
        }
    };
}

template<>
struct std::hash<Engine::RenderTaskTraits>
{
    size_t operator()( const Engine::RenderTaskTraits& target ) const
    {
        static std::hash<uint32_t> domain_hasher;
        size_t                     return_value = domain_hasher( target.domain );
        boost::hash_combine( return_value, target.rtti.hash_code() );
        return return_value;
    }
};
#endif

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
        void RegisterRenderPass( const std::wstring_view name, IRenderPassTaskFactory *task );
        void RenderPassWith( const std::wstring_view name, const eShaderDomain domain );

        void UnregisterRenderInstance( const std::wstring_view name );
        void UnregisterRenderPass( const std::wstring_view name );
        void RenderPassWithout( const std::wstring_view name, const eShaderDomain domain );

		void RegisterStructuredBuffer(const StructuredBufferDecorator* sb);
		void UnregisterStructuredBuffer(const StructuredBufferDecorator* sb);

		void RegisterContextPreRenderSetup(const std::string_view name, const ContextSetupFunction& prerender_func);
		void UnregisterContextPreRenderSetup(const std::string_view name);
		void RegisterContextPostRenderSetup(const std::string_view name, const ContextSetupFunction& postrender_func);
        void UnregisterContextPostRenderSetup( const std::string_view name );

		using RenderPassTaskFactoryContainer = std::unordered_map<std::wstring, Unique<IRenderPassTaskFactory>>;
		using RenderPassTaskBorrowedContainer = std::unordered_map<std::wstring, IRenderPassTaskFactory*>;
        using RenderPassTaskInstantiatedContainer = std::unordered_map<HashType, std::vector<RenderPassTask*>>;
        using RenderPassTaskUsageContainer = std::unordered_map<HashType, size_t>;

    private:
        void RenderPass(
                IRenderPassTaskFactory                                           *factory,
                float                                                             dt,
                bool                                                              shader_bypass,
                eShaderDomain                                                     domain,
                const Graphics::SBs::LocalParamSB                                &local_param_sb,
                const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                const ObjectPredication                                          &predication,
                const ContextSetupFunction                                       &prerender_predicate,
                const ContextSetupFunction                                       &postrender_predicate,
                const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_funcs,
                const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_funcs )
        {
            const HashType type            = factory->GetTaskType();
            if (!m_render_pass_tasks_usage_[domain].contains(type))
            {
                m_render_pass_tasks_usage_[ domain ].emplace( type, 0 );
            }

            size_t         use_count       = m_render_pass_tasks_usage_[ domain ][ type ];
            const size_t   allocated_count = m_render_pass_tasks_[ domain ][ type ].size();

            if (use_count == allocated_count)
            {
                m_render_pass_tasks_[ domain ][ type ].emplace_back( factory->New() );
            }

            RenderPassTask* task = m_render_pass_tasks_[ domain ][ type ].at( use_count );

            task->Cleanup();
            task->PreRun( m_render_candidates_, m_render_candidates_->size(), predication );
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
            use_count++;
        }

        void RenderPassAssisted( IRenderPassTaskFactory             *factory,
                                 float                              dt,
                                 bool                               shader_bypass,
                                 eShaderDomain                      domain,
                                 const Graphics::SBs::LocalParamSB &local_param_sb,
                                 const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                 const ObjectPredication                                 &predication,
                                 const ContextSetupFunction                              &prerender_predicate,
                                 const ContextSetupFunction                              &postrender_predicate )
        {
            RenderPass( factory,
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
#if IS_DLL
            constexpr static bool       Inclusion = false;
            constexpr static std::array value     = { ExcludeRenderTaskTs::StaticTypeHash()... };

            static const std::vector<HashType>& GetValue()
            {
                static std::vector<HashType> container( value.begin(), value.end() );
                return container;
            }
#else
            static void ResolveDirty( const RenderPassTaskFactoryContainer& cont,
                                      std::vector<IRenderPassTaskFactory*>& factories )
            {
                factories.clear();
                factories.reserve( cont.size() );

                for ( const Unique<IRenderPassTaskFactory>& factory : cont | std::views::values )
                {
                    if ( bool check[] = { factory->GetTaskType() == ExcludeRenderTaskTs::StaticTypeHash()... };
                         std::any_of(
                                 std::begin( check ), std::end( check ), []( const bool b ) { return b == true; } ) )
                    {
                        continue;
                    }

                    if ( factory != nullptr )
                    {
                        factories.emplace_back( factory.get() );
                    }
                }
            }
#endif
		};

		template <typename... IncludeRenderTaskTs>
        struct InclusionPredicate
        {
#if IS_DLL
            constexpr static bool       Inclusion = true;
            constexpr static std::array value     = { IncludeRenderTaskTs::StaticTypeHash()... };

            static const std::vector<HashType>& GetValue()
            {
                static std::vector<HashType> container( value.begin(), value.end() );
                return container;
            }
#else
            static void ResolveDirty( const RenderPassTaskFactoryContainer& cont,
                                      std::vector<IRenderPassTaskFactory*>& factories )
            {
                factories.clear();
                factories.reserve( cont.size() );

                for ( const Unique<IRenderPassTaskFactory>& factory : cont | std::views::values )
                {
                    if ( bool check[] = { factory->GetTaskType() == IncludeRenderTaskTs::StaticTypeHash()... };
                         std::any_of(
                                 std::begin( check ), std::end( check ), []( const bool b ) { return b == true; } ) )
                    {
                        factories.emplace_back( factory.get() );
                    }
                }
            }
#endif
        };

#if IS_DLL
        using BorrowedRenderPassFactories = std::vector<IRenderPassTaskFactory*>;
        BorrowedRenderPassFactories& GetRenderTasks( const RenderTaskTraits& traits, const std::vector<HashType>& types, bool inclusion );
        BorrowedRenderPassFactories& GetFactories( const RenderTaskTraits& traits );
        void ResolveDirty( const std::vector<HashType>& types, BorrowedRenderPassFactories& tasks, bool inclusion );
#endif

    public:

	    template <typename PredicationT, eShaderDomain Domain>
        void RenderPassVanillaPredicate(
            float                                                             dt,
            bool                                                              shader_bypass,
            const Graphics::SBs::LocalParamSB&                                local_param_sb,
            const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
            const ObjectPredication&                                          predication,
            const ContextSetupFunction&                                       prerender_predicate,
            const ContextSetupFunction&                                       postrender_predicate,
            const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_funcs,
            const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_funcs
        )
	    {
#if !IS_DLL
            static std::vector<IRenderPassTaskFactory*> borrowed_factories_;
            static std::once_flag                       delegate_flag;

            static const auto& func = [ this ]()
            { PredicationT::ResolveDirty( m_unique_render_pass_task_factories_, borrowed_factories_ ); };

            std::call_once( delegate_flag,
                            [ this ]()
                            {
                                func();
                                onRenderTaskDirty.Listen( func );
                            } );
#else
            static RenderTaskTraits                     traits{ typeid( PredicationT ), Domain };
            static std::vector<IRenderPassTaskFactory*> borrowed_factories_ =
                    GetRenderTasks( traits, PredicationT::GetValue(), PredicationT::Inclusion );
#endif

            for (IRenderPassTaskFactory* factory : borrowed_factories_)
            {
                RenderPass( factory,
                            dt,
                            shader_bypass,
                            Domain,
                            local_param_sb,
                            additional_sbs,
                            predication,
                            prerender_predicate,
                            postrender_predicate,
                            prerender_funcs,
                            postrender_funcs );
            }
	    }

	    template <typename PredicationT, eShaderDomain Domain>
        void RenderPassAssistedPredicate( float                                                    dt,
                                          bool                                                     shader_bypass,
                                          const Graphics::SBs::LocalParamSB                       &local_param_sb,
                                          const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                          const ObjectPredication                                 &predication,
                                          const ContextSetupFunction                              &prerender_predicate,
                                          ContextSetupFunction &postrender_predicate
        )
	    {
#if !IS_DLL
            static std::vector<IRenderPassTaskFactory*> borrowed_factories_;
            static std::once_flag                       delegate_flag;

            static const auto& func = [ this ]()
            { PredicationT::ResolveDirty( m_unique_render_pass_task_factories_, borrowed_factories_ ); };

            std::call_once( delegate_flag,
                            [ this ]()
                            {
                                func();
                                onRenderTaskDirty.Listen( func );
                            } );
#else
            static RenderTaskTraits                     traits{ typeid( PredicationT ), Domain };
            static std::vector<IRenderPassTaskFactory*> borrowed_factories_ =
                    GetRenderTasks( traits, PredicationT::GetValue(), PredicationT::Inclusion );
#endif

            for ( IRenderPassTaskFactory* factory : borrowed_factories_ )
            {
                RenderPassAssisted( factory,
                                    dt,
                                    shader_bypass,
                                    Domain,
                                    local_param_sb,
                                    additional_sbs,
                                    predication,
                                    prerender_predicate,
                                    postrender_predicate );
            }
	    }

		template <eShaderDomain Domain, typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void
        RenderPassVanillaExclusion(
			    float                                                    dt,
				bool                                                     shader_bypass,
				const Graphics::SBs::LocalParamSB& local_param_sb,
				const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
				const ObjectPredication& predication,
				const ContextSetupFunction& prerender_predicate,
				const ContextSetupFunction& postrender_predicate,
				const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_funcs,
				const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_funcs)
		{
            RenderPassVanillaPredicate<ExclusionPredicate<RenderTaskTs...>, Domain>( dt,
                                                                                     shader_bypass,
                                                                                     local_param_sb,
                                                                                     additional_sbs,
                                                                                     predication,
                                                                                     prerender_predicate,
                                                                                     postrender_predicate,
                                                                                     prerender_funcs,
                                                                                     postrender_funcs );
		}

		template <eShaderDomain Domain, typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void
        RenderPassVanillaInclusion( float                                                    dt,
                                   bool                                                     shader_bypass,
                                   const Graphics::SBs::LocalParamSB                       &local_param_sb,
                                   const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                   const ObjectPredication                                 &predication,
                                   const ContextSetupFunction                              &prerender_predicate,
                                   const ContextSetupFunction                              &postrender_predicate,
                                   const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_funcs,
                                   const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_funcs )
        {
            RenderPassVanillaPredicate<InclusionPredicate<RenderTaskTs...>, Domain>( dt,
                                                                                     shader_bypass,
                                                                                     local_param_sb,
                                                                                     additional_sbs,
                                                                                     predication,
                                                                                     prerender_predicate,
                                                                                     postrender_predicate,
                                                                                     prerender_funcs,
                                                                                     postrender_funcs );
        }

		template <eShaderDomain Domain, typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
		void RenderPassAssistedInclusion(
			float                                                    dt,
			bool                                                     shader_bypass,
			const Graphics::SBs::LocalParamSB& local_param_sb,
			const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
			const ObjectPredication& predication,
			const ContextSetupFunction& prerender_predicate,
			const ContextSetupFunction& postrender_predicate) const
		{
            RenderPassAssistedPredicate<InclusionPredicate<RenderTaskTs...>, Domain>( dt,
                                                                                      shader_bypass,
                                                                                      local_param_sb,
                                                                                      additional_sbs,
                                                                                      predication,
                                                                                      prerender_predicate,
                                                                                      postrender_predicate,
                                                                                      m_prerender_funcs_,
                                                                                      m_postrender_funcs_ );
        }

		template <eShaderDomain Domain, typename... RenderTaskTs>
            requires( std::is_base_of_v<RenderPassTask, RenderTaskTs>, ... )
        void RenderPassAssistedExclusion( float                                                    dt,
                                          bool                                                     shader_bypass,
                                          const Graphics::SBs::LocalParamSB                       &local_param_sb,
                                          const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                          const ObjectPredication                                 &predication,
                                          const ContextSetupFunction                              &prerender_predicate,
                                          const ContextSetupFunction &postrender_predicate ) const
        {
            RenderPassAssistedPredicate<ExclusionPredicate<RenderTaskTs...>, Domain>( dt,
                                                                                      shader_bypass,
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
	    
		std::unordered_map<std::wstring, Unique<RenderInstanceTask>>  m_render_instance_tasks_;
        RenderPassTaskFactoryContainer                                m_unique_render_pass_task_factories_;
        RenderPassTaskBorrowedContainer                               m_render_pass_tasks_factories_[ SHADER_DOMAIN_MAX ];
        
        RenderPassTaskInstantiatedContainer m_render_pass_tasks_[ SHADER_DOMAIN_MAX ];
        RenderPassTaskUsageContainer        m_render_pass_tasks_usage_[ SHADER_DOMAIN_MAX ];

#if IS_DLL
        using BorrowedRenderTasksContainer = std::unordered_map<RenderTaskTraits, BorrowedRenderPassFactories>;
        BorrowedRenderTasksContainer m_render_task_resolvers_;
#endif

		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
