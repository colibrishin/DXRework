#pragma once
#include "Renderer.h"

#include "RaytracingRenderer.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_RENDERPIPELINE_API RaytracingRenderer : public Abstracts::Singleton<RaytracingRenderer>
	{
		GENERATE_BODY
	public:
		explicit RaytracingRenderer(SINGLETON_LOCK_TOKEN)
            : m_b_ready_(false) {}

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

	protected:
	    ~RaytracingRenderer() override;
	    
	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		
		bool m_b_ready_;
		std::unordered_map<std::string_view, ContextSetupFunction> m_prerender_funcs_;
		std::unordered_map<std::string_view, ContextSetupFunction> m_postrender_funcs_;
		aligned_vector<const StructuredBufferDecorator*> m_additional_sbs_;
		std::unordered_map<std::wstring, Unique<RenderInstanceTask>> m_render_instance_tasks_;
		std::unordered_map<std::wstring, Unique<RenderPassTask>> m_render_pass_tasks_;

		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	    
	};
}
