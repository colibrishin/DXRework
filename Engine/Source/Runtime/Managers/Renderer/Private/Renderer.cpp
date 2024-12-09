#include "../Public/Renderer.h"

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Managers/SceneManager/Public/SceneManager.hpp"

namespace Engine::Managers
{
	Renderer::~Renderer() 
	{
		for (const RenderInstanceTask* task : m_render_instance_tasks_) 
		{
			delete task;
		}

		for (const RenderPassTask* task : m_render_pass_tasks_) 
		{
			delete task;
		}

		for (const RenderPassPrequisiteTask* task : m_render_prequisite_tasks_) 
		{
			delete task;
		}
	}

	void Renderer::PreUpdate(const float& dt)
	{
		for (RenderPassTask* task : m_render_pass_tasks_)
		{
			task->Cleanup();
		}

		for (size_t i = 0; i < m_render_instance_tasks_.size(); ++i) 
		{
			if (RenderMap::accesor acc;
				m_render_candidates_.find(i, acc)) 
			{
				m_render_instance_tasks_[i]->Cleanup(acc.second);
			}
		}

		m_b_ready_ = false;
	}

	void Renderer::Update(const float& dt) {}

	void Renderer::FixedUpdate(const float& dt) {}

	void Renderer::PreRender(const float& dt)
	{
		// Pre-processing, Mapping the materials to the model renderers.
		if (const auto& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock()) 
		{
			for (RenderInstanceTask* task : m_render_instance_tasks_)
			{
				task->Run(scene.get(), m_render_candidates_, SHADER_DOMAIN_MAX, m_instance_count_);
			}
		}

		m_b_ready_ = true;
	}

	void Renderer::Render(const float& dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			RenderPass(static_cast<eShaderDomain>(i));
		}
	}

	void Renderer::RenderPass(const eShaderDomain domain) 
	{
		for (RenderPassTask* task : m_render_pass_tasks_) 
		{
			if (RenderMap::accessor acc;
				m_render_candidates_.find(domain, acc)) 
			{
				task->Run(
					domain,
					acc.second,
					m_instance_count_,
					m_render_prequisite_tasks_.data(),
					m_render_prequisite_tasks_.size());
			}
		}
	}

	void Renderer::PostRender(const float& dt) {}

	void Renderer::PostUpdate(const float& dt) {}

	void Renderer::Initialize() {}

	void Renderer::RegisterRenderInstance(const RenderInstanceTask* task)
	{
		if (task != nullptr) 
		{
			m_render_instance_tasks_.push_back(task);
		}
	}

	void Renderer::RegisterRenderPass(const RenderPassTask* task)
	{
		if (task != nullptr)
		{
			m_render_pass_tasks_.push_back(task);
		}
	}

	void Renderer::RegisterRenderPassPrequisite(const RenderPassPrequisiteTask* task)
	{
		if (task != nullptr) 
		{
			m_render_prequisite_tasks_.push_back(task);
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
