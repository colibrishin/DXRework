#pragma once
#include <atomic>
#include <ranges>

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "RenderTask.h"
#include "Delegation/Public/Delegation.hpp"

#include "Renderer.generated.h"

DEFINE_DELEGATE(OnRenderDone, const Engine::eShaderDomain);

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
		
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Initialize() override;
		
		void RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task);
		void RegisterRenderPass(const std::wstring_view name, RenderPassTask* task);
		void UnregisterRenderInstance(const std::wstring_view name);
		void UnregisterRenderPass(const std::wstring_view name);

		void RegisterStructuredBuffer(const StructuredBufferDecorator* sb);
		void UnregisterStructuredBuffer(const StructuredBufferDecorator* sb);

		void RegisterContextPreRenderSetup(const std::string_view name, const ContextSetupFunction& prerender_func);
		void UnregisterContextPreRenderSetup(const std::string_view name);
		void RegisterContextPostRenderSetup(const std::string_view name, const ContextSetupFunction& postrender_func);
		void UnregisterContextPostRenderSetup(const std::string_view name);

	    template <typename... ExcludeRenderTaskTs> requires (std::is_base_of_v<RenderPassTask, ExcludeRenderTaskTs>, ...)
	    void RenderPassValinaExclusion(
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
        ) const
	    {
	        static std::vector<RenderPassTask*> render_pass_task_copy;

	        if (m_b_task_dirty_)
	        {
	            render_pass_task_copy.clear();
	            render_pass_task_copy.reserve( m_render_pass_tasks_.size() );

	            for (const auto& task : m_render_pass_tasks_ | std::views::values)
	            {
	                if (bool check[] = { task->GetTypeHash() == ExcludeRenderTaskTs::StaticTypeHash()... };
                        std::any_of( std::begin( check ), std::end( check ), [](const bool b) { return b == true; } ))
	                {
	                    continue;
	                }

	                if (task != nullptr)
	                {
	                    render_pass_task_copy.emplace_back( task.get() );   
	                }
	            }
	        }

	        for (const auto& task : render_pass_task_copy)
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
                          postrender_funcs);
	        }
	    }

	    template <typename... ExcludeRenderTaskTs> requires (std::is_base_of_v<RenderPassTask, ExcludeRenderTaskTs>, ...)
	    void RenderPassAssistedExclusion(
            float                                                   dt,
            bool                                                    shader_bypass,
            eShaderDomain                                           domain,
            const Graphics::SBs::LocalParamSB&                      local_param_sb,
            const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
            const ObjectPredication&                                predication,
            const ContextSetupFunction&                             prerender_predicate,
            const ContextSetupFunction&                             postrender_predicate
        ) const
	    {
	        static std::vector<RenderPassTask*> render_pass_task_copy;

	        if (m_b_task_dirty_)
	        {
	            render_pass_task_copy.clear();
	            render_pass_task_copy.reserve( m_render_pass_tasks_.size() );

	            for (const auto& task : m_render_pass_tasks_ | std::views::values)
	            {
	                if (bool check[] = { task->GetTypeHash() == ExcludeRenderTaskTs::StaticTypeHash()... };
                        std::any_of( std::begin( check ), std::end( check ), [](const bool b) { return b == true; } ))
	                {
	                    continue;
	                }

	                if (task != nullptr)
	                {
	                    render_pass_task_copy.emplace_back( task.get() );   
	                }
	            }
	        }

	        for (const auto& task : render_pass_task_copy)
	        {
	            task->Run( dt,
                          shader_bypass,
                          &m_render_candidates_[ domain ],
                          additional_sbs,
                          local_param_sb,
                          predication,
                          prerender_predicate,
                          postrender_predicate,
                          m_prerender_funcs_,
                          m_postrender_funcs_);
	        }
	    }

		void RenderPassVanilla(
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
		) const;
		
		void RenderPassAssisted(
			float                                                   dt,
			bool                                                    shader_bypass,
			eShaderDomain                                           domain,
			const Graphics::SBs::LocalParamSB&                      local_param_sb,
			const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
			const ObjectPredication&                                predication,
			const ContextSetupFunction&                             prerender_predicate,
			const ContextSetupFunction&                             postrender_predicate
		) const;

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
		std::unordered_map<std::wstring, Unique<RenderPassTask>> m_render_pass_tasks_;
	    
		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
