#include "../Public/ParticleRenderer.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"
#include "ParticleRenderer.generated.h"

namespace Engine::Components
{
	COMP_CLONE_IMPL(ParticleRenderer)

	ParticleRenderer::ParticleRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: RenderComponent(owner),
		  m_b_follow_owner_(true) {}

	ParticleRenderer::ParticleRenderer(const ParticleRenderer& other)
		: RenderComponent(other)
	{
		m_cs_               = other.m_cs_;
		m_cs_meta_path_ = other.m_cs_meta_path_;
		m_instances_        = other.m_instances_;
		m_b_follow_owner_   = other.m_b_follow_owner_;
	}

	ParticleRenderer& ParticleRenderer::operator=(const ParticleRenderer& other)
	{
		if (this != &other)
		{
			RenderComponent::operator=(other);
			m_cs_               = other.m_cs_;
			m_cs_meta_path_ = other.m_cs_meta_path_;
			m_instances_        = other.m_instances_;
			m_b_follow_owner_   = other.m_b_follow_owner_;
		}
		return *this;
	}

	void ParticleRenderer::Initialize()
	{
		RenderComponent::Initialize();
		SetCount(1);
		SetSize(1.f);
	}

	void ParticleRenderer::Update(const float dt)
	{
		if (m_cs_ && GetShape().lock())
		{
			GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
			const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, true, L"Particle Renderer Update");
			const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
			
			CheckSize<UINT>(m_instances_.size(), L"Warning: Particle instance size is too much for structured buffer!");

			primitive.commandList->SoftReset();
			m_sb_buffer_->SetData(&primitive, static_cast<UINT>(m_instances_.size()), m_instances_.data());
			
			m_sb_buffer_->TransitionToUAV(&primitive);
			m_sb_buffer_->CopyUAVHeap(&primitive);

			const auto thread      = m_cs_->GetThread();
			const auto flatten     = thread[0] * thread[1] * thread[2];
			const UINT group_count = static_cast<UINT>(m_instances_.size() / flatten);
			const UINT remainder   = static_cast<UINT>(m_instances_.size() % flatten);

			const UINT groups[3] = {group_count + (remainder ? 1 : 0), 1, 1};
			m_cs_->Dispatch(&primitive, groups, m_params_);
			primitive.commandList->Execute();

			const GraphicInterfaceContextReturnType& copy_context = gi.GetNewContext(0, true, L"Particle Renderer Update");
			const GraphicInterfaceContextPrimitive& copy_primitive = copy_context.GetPointers();
			m_sb_buffer_->GetData(&copy_primitive, static_cast<UINT>(m_instances_.size()), m_instances_.data());

			// Remove inactive particles.
			for (auto it = m_instances_.begin(); it != m_instances_.end();)
			{
				if (!it->GetActive())
				{
					it = m_instances_.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	}

	void ParticleRenderer::PreUpdate(const float dt) {}

	void ParticleRenderer::FixedUpdate(const float dt) {}

	void ParticleRenderer::OnSerialized()
	{
		RenderComponent::OnSerialized();
	}

	void ParticleRenderer::OnDeserialized()
	{
		RenderComponent::OnDeserialized();

		if (const auto cs = Resources::ComputeShader::GetByMetadataPath(m_cs_meta_path_).lock())
		{
			m_cs_ = cs;
		}
	}

	eComponentUpdatePriority ParticleRenderer::GetUpdatePriority() const
	{
		return COM_PRIORITY_RENDER;
	}

	aligned_vector<Graphics::SBs::InstanceSB> ParticleRenderer::GetParticles()
	{
		std::lock_guard lock(m_instances_mutex_);
		return reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB>&>(m_instances_);
	}

	void ParticleRenderer::SetFollowOwner(const bool follow)
	{
		m_b_follow_owner_ = follow;
	}

	void ParticleRenderer::SetCount(const size_t count)
	{
		std::lock_guard lock(m_instances_mutex_);
		// Expand and apply the world matrix of the owner to each instance.
		m_instances_.resize(count);
		m_params_.SetParam(particle_count_slot, static_cast<int>(count));
	}

	void ParticleRenderer::SetDuration(const float duration)
	{
		std::lock_guard lock(m_instances_mutex_);
		m_params_.SetParam(duration_slot, duration);

		for (auto& instance : m_instances_)
		{
			instance.SetLife(duration);
		}
	}

	void ParticleRenderer::SetSize(const float size)
	{
		m_params_.SetParam(size_slot, size);
	}

	bool ParticleRenderer::IsFollowOwner() const
	{
		return m_b_follow_owner_;
	}

	ParticleRenderer::ParticleRenderer()
		: RenderComponent(),
		  m_b_follow_owner_(true) {}

	void ParticleRenderer::SetComputeShader(const Weak<Resources::ComputeShader>& cs)
	{
		if (const auto shader = cs.lock())
		{
			m_cs_               = shader;
			m_cs_meta_path_ = shader->GetMetadataPath().string();
		}
	}
}
