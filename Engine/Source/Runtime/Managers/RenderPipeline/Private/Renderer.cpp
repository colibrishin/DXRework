#include "../Public/Renderer.h"

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"
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
				ptr->Run
					(
					 scene.get(),
					 m_render_candidates_,
					 SHADER_DOMAIN_MAX,
					 m_instance_count_
					);
			}
		}

		m_b_ready_ = true;
	}

	void Renderer::Render(const float dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			RenderPass(dt, false, static_cast<eShaderDomain>(i), {}, {}, {}, {});
		}
	}

	void Renderer::RenderPass(
		const float                                        dt, 
		const bool                                         shader_bypass, 
		const eShaderDomain                                domain,
		const SBs::LocalParamSB&                           local_param_sb,
		const ObjectPredication&                           predication,
		const ContextSetupFunction&						   prerender_predicate,
		const ContextSetupFunction&						   postrender_predicate) const
	{
		for (const auto& ptr : m_render_pass_tasks_ | std::views::values) 
		{
			ptr->Run(
			          dt,
			          shader_bypass,
			          &m_render_candidates_[domain],
			          local_param_sb,
			          m_instance_count_,
			          predication,
			          prerender_predicate,
			          postrender_predicate);
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

	bool Renderer::Ready() const
	{
		return m_b_ready_;
	}

	uint64_t Renderer::GetInstanceCount() const
	{
		return m_instance_count_.load();
	}
}
