#include "../Public/Renderer.h"

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"
#include "../Public/RenderPipeline.h"

namespace Engine::Managers
{
	Renderer::~Renderer() {}

	void Renderer::PreUpdate(const float dt)
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

	void Renderer::Update(const float dt) {}

	void Renderer::FixedUpdate(const float dt) {}

	void Renderer::PreRender(const float dt)
	{
		if (const auto& scene = SceneManager::GetInstance().GetActiveScene().lock()) 
		{
			for (const auto& ptr : m_render_instance_tasks_ | std::views::values)
			{
				ptr->Run(scene.get(), m_render_candidates_, SHADER_DOMAIN_MAX);
			}
		}

		m_b_ready_ = true;
	}

	void Renderer::Render(const float dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			RenderPassAssisted(dt, false, static_cast<eShaderDomain>(i), {}, m_additional_sbs_, {}, {}, {});
		}
	}

	void Renderer::RenderPassAssisted(
		float                                                   dt,
		bool                                                    shader_bypass,
		eShaderDomain                                           domain,
		const SBs::LocalParamSB&                                local_param_sb,
		const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
		const ObjectPredication&                                predication,
		const ContextSetupFunction&                             prerender_predicate,
		const ContextSetupFunction&                             postrender_predicate,
		const bool call_cleanup
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

		if (call_cleanup)
		{
			for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
			{
				ptr->Cleanup();
			}
		}
	}

	void Renderer::PostRender(const float dt) {}

	void Renderer::PostUpdate(const float dt) {}

	void Renderer::Initialize() {}

	void Renderer::RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task)
	{
		if (task != nullptr) 
		{
			m_render_instance_tasks_.emplace(name, std::unique_ptr<RenderInstanceTask>(task));
		}
	}

	void Renderer::RegisterRenderPass(const std::wstring_view name, RenderPassTask* task)
	{
		if (task != nullptr)
		{
			m_render_pass_tasks_.emplace(name, std::unique_ptr<RenderPassTask>(task));
		}
	}

	void Renderer::UnregisterRenderInstance(const std::wstring_view name)
	{
		if (m_render_instance_tasks_.contains(name.data()))
		{
			m_render_instance_tasks_.erase(name.data());
		}
	}

	void Renderer::UnregisterRenderPass(const std::wstring_view name)
	{
		if (m_render_pass_tasks_.contains(name.data()))
		{
			m_render_pass_tasks_.erase(name.data());
		}
	}

	void Renderer::RegisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		if (const auto& it = std::ranges::find(m_additional_sbs_, sb);
			it == m_additional_sbs_.end())
		{
			m_additional_sbs_.push_back(sb);
		}
	}
	
	void Renderer::UnregisterStructuredBuffer(const StructuredBufferDecorator* sb)
	{
		std::erase_if(m_additional_sbs_, [&sb](const StructuredBufferDecorator* elem)
		{
			return elem == sb;
		});
	}

	void Renderer::RegisterContextPreRenderSetup(
		const std::string_view name, const ContextSetupFunction& prerender_func)
	{
		if (!m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.emplace(name, prerender_func);	
		}
	}

	void Renderer::UnregisterContextPreRenderSetup(const std::string_view name)
	{
		if (m_prerender_funcs_.contains(name))
		{
			m_prerender_funcs_.erase(name);
		}
	}

	void Renderer::RegisterContextPostRenderSetup(
		const std::string_view name, const ContextSetupFunction& postrender_func)
	{
		if (!m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.erase(name);
		}
	}

	void Renderer::UnregisterContextPostRenderSetup(const std::string_view name)
	{
		if (m_postrender_funcs_.contains(name))
		{
			m_postrender_funcs_.erase(name);
		}
	}

	void Renderer::RenderPassVanilla(
		float dt, bool shader_bypass, eShaderDomain domain,
		const SBs::LocalParamSB& local_param_sb,
		const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
		const ObjectPredication& predication,
		const ContextSetupFunction& prerender_predicate,
		const ContextSetupFunction& postrender_predicate,
		const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_funcs,
		const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_funcs,
		const bool call_cleanup
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

		if (call_cleanup)
		{
			for (const auto& ptr : m_render_pass_tasks_ | std::views::values)
			{
				ptr->Cleanup();
			}
		}
	}

	bool Renderer::Ready() const
	{
		return m_b_ready_;
	}
}
