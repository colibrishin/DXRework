#include "../Public/Renderer.h"

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"
#include "../Public/RenderPipeline.h"

namespace Engine::Managers
{
	Renderer::~Renderer() {}

	void Renderer::PreUpdate(const float& dt)
	{
		for (RenderPassTask* task : m_render_pass_tasks_)
		{
			task->Cleanup();
		}

		for (size_t i = 0; i < m_render_instance_tasks_.size(); ++i) 
		{
			for (size_t j = 0; j < SHADER_DOMAIN_MAX; ++j) 
			{
				if (RenderMap::accessor acc;
					m_render_candidates_[j].find(acc, i))
				{
					m_render_instance_tasks_[i]->Cleanup(&acc->second);
				}
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
			for (size_t i = 0; i < m_render_instance_tasks_.size(); ++i)
			{
				for (size_t j = 0; j < SHADER_DOMAIN_MAX; ++j) 
				{
					if (RenderMap::accessor acc;
						m_render_candidates_[j].find(acc, i))
					{
						m_render_instance_tasks_[i]->Run(
							scene.get(), 
							&acc->second, 
							m_instance_count_);
					}
				}
			}
		}

		m_b_ready_ = true;
	}

	void Renderer::Render(const float& dt)
	{
		for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
		{
			RenderPass(dt, false, static_cast<eShaderDomain>(i), {}, {}, {});
		}
	}

	void Renderer::RenderPass(
		const float                                        dt, 
		const bool                                         shader_bypass, 
		const eShaderDomain                                domain,
		const Graphics::SBs::LocalParamSB&                 local_param_sb,
		const aligned_vector<RenderPassPrerequisiteTask*>& additional_task,
		const ObjectPredication&                           predication) 
	{
		const size_t default_offset = m_render_prerequisite_tasks_.size();
		m_render_prerequisite_tasks_.insert(m_render_prerequisite_tasks_.end(), additional_task.begin(), additional_task.end());

		for (RenderPassTask* task : m_render_pass_tasks_) 
		{
			task->Run(
				dt,
				shader_bypass,
				domain,
				&m_render_candidates_[domain],
				local_param_sb,
				m_instance_count_,
				m_render_prerequisite_tasks_.data(),
				m_render_prerequisite_tasks_.size(),
				predication); // todo: culling
		}

		m_render_prerequisite_tasks_.erase(m_render_prerequisite_tasks_.begin() + default_offset, m_render_prerequisite_tasks_.end());
	}

	void Renderer::PostRender(const float& dt) {}

	void Renderer::PostUpdate(const float& dt) {}

	void Renderer::Initialize() {}

	void Renderer::RegisterRenderInstance(RenderInstanceTask* task)
	{
		if (task != nullptr) 
		{
			m_render_instance_tasks_.push_back(task);
		}
	}

	void Renderer::RegisterRenderPass(RenderPassTask* task)
	{
		if (task != nullptr)
		{
			m_render_pass_tasks_.push_back(task);
		}
	}

	void Renderer::RegisterRenderPassPrerequisite(RenderPassPrerequisiteTask* task)
	{
		if (task != nullptr) 
		{
			m_render_prerequisite_tasks_.push_back(task);
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
