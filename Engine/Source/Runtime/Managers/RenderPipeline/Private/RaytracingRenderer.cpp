#include "RaytracingRenderer.h"
#include "RaytracingRenderer.generated.h"

#include "SceneManager/Public/SceneManager.h"
#include <ranges>

namespace Engine::Managers
{
	RaytracingRenderer::~RaytracingRenderer() {}

	void RaytracingRenderer::PreUpdate(const float dt)
	{
		for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
		{
			ptr->Cleanup();
		}

		for (const auto& ptr : m_render_instance_tasks_ | std::views::values) 
		{
		    ptr->Cleanup(m_render_candidates_, SHADER_DOMAIN_MAX);
		}

		m_b_ready_ = false;
	}

	void RaytracingRenderer::Update(const float dt) {}

	void RaytracingRenderer::FixedUpdate(const float dt) {}

	void RaytracingRenderer::PreRender(const float dt)
	{
		if (const auto& scene = SceneManager::GetInstance().GetActiveScene().lock()) 
		{
			for (const auto& ptr : m_render_instance_tasks_ | std::views::values)
			{
				ptr->Run(scene.get(), m_render_candidates_, SHADER_DOMAIN_MAX);
			}
		}

	    for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
	    {
	        ptr->PreRun(m_render_candidates_, std::size(m_render_candidates_), {});
	    }

	    m_b_ready_ = true;
	}

	void RaytracingRenderer::Render(const float dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			RenderPassAssisted(dt, false, static_cast<eShaderDomain>(i), {}, m_additional_sbs_, {}, {}, {});
			onRenderDone.Broadcast(static_cast<eShaderDomain>(i));
		}
	}

	void RaytracingRenderer::RenderPassAssisted(
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
		for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
		{
			ptr->Run
					(
					 dt,
					 shader_bypass,
					 &m_render_candidates_[domain],
					 additional_sbs,
					 local_param_sb,
					 predication,
					 prerender_predicate,
					 postrender_predicate,
					 m_prerender_funcs_,
					 m_postrender_funcs_
					);
		}
	}

	void RaytracingRenderer::PostRender(const float dt) {}

	void RaytracingRenderer::PostUpdate(const float dt) {}

	void RaytracingRenderer::Initialize() {}

	void RaytracingRenderer::RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task)
	{
		if (task != nullptr) 
		{
			m_render_instance_tasks_.emplace(name, std::unique_ptr<RenderInstanceTask>(task));
		}
	}

	void RaytracingRenderer::RegisterRenderPass(const std::wstring_view name, RenderPassTask* task)
	{
		if (task != nullptr)
		{
			m_render_pass_tasks_.emplace(name, std::unique_ptr<RenderPassTask>(task));
		}
	}

	void RaytracingRenderer::UnregisterRenderInstance(const std::wstring_view name)
	{
		if (m_render_instance_tasks_.contains(name.data()))
		{
			m_render_instance_tasks_.erase(name.data());
		}
	}

	void RaytracingRenderer::UnregisterRenderPass(const std::wstring_view name)
	{
		if (m_render_pass_tasks_.contains(name.data()))
		{
			m_render_pass_tasks_.erase(name.data());
		}
	}

	void RaytracingRenderer::RegisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		if (const auto& it = std::ranges::find(m_additional_sbs_, sb);
			it == m_additional_sbs_.end())
		{
			m_additional_sbs_.push_back(sb);
		}
	}
	
	void RaytracingRenderer::UnregisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		std::erase_if(m_additional_sbs_, [&sb](const StructuredBufferDecorator* elem)
		{
			return elem == sb;
		});
	}

	void RaytracingRenderer::RegisterContextPreRenderSetup(
		const std::string_view name, const ContextSetupFunction& prerender_func)
	{
		if (!m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.emplace(name, prerender_func);	
		}
	}

	void RaytracingRenderer::UnregisterContextPreRenderSetup(const std::string_view name)
	{
		if (m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.erase(name);
		}
	}

	void RaytracingRenderer::RegisterContextPostRenderSetup(
		const std::string_view name, const ContextSetupFunction& postrender_func)
	{
		if (!m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.emplace(name, postrender_func);
		}
	}

	void RaytracingRenderer::UnregisterContextPostRenderSetup(const std::string_view name)
	{
		if (m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.erase(name);
		}
	}

	void RaytracingRenderer::RenderPassVanilla(
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
		for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
		{
			ptr->Run
					(
					 dt,
					 shader_bypass,
                     &m_render_candidates_[domain],
					 additional_sbs,
					 local_param_sb,
					 predication,
					 prerender_predicate,
					 postrender_predicate,
					 prerender_funcs,
					 postrender_funcs
					);
		}
	}

	bool RaytracingRenderer::Ready() const
	{
		return m_b_ready_;
	}
}
