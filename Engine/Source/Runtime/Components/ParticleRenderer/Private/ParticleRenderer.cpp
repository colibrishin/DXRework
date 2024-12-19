#include "../Public/ParticleRenderer.h"

#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"

namespace Engine::Components
{
	COMP_CLONE_IMPL(ParticleRenderer)

	ParticleRenderer::ParticleRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: RenderComponent(RENDER_COM_T_PARTICLE, owner),
		  m_b_follow_owner_(true) {}

	ParticleRenderer::ParticleRenderer(const ParticleRenderer& other)
		: RenderComponent(other)
	{
		m_cs_               = other.m_cs_;
		m_cs_meta_path_str_ = other.m_cs_meta_path_str_;
		m_instances_        = other.m_instances_;
		m_b_follow_owner_   = other.m_b_follow_owner_;
	}

	ParticleRenderer& ParticleRenderer::operator=(const ParticleRenderer& other)
	{
		if (this != &other)
		{
			RenderComponent::operator=(other);
			m_cs_               = other.m_cs_;
			m_cs_meta_path_str_ = other.m_cs_meta_path_str_;
			m_instances_        = other.m_instances_;
			m_b_follow_owner_   = other.m_b_follow_owner_;
		}
		return *this;
	}

	void ParticleRenderer::Initialize()
	{
		RenderComponent::Initialize();

		const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Particle Renderer Init").lock();
		cmd->SoftReset();
		m_local_param_buffer_.Create(cmd->GetList(), 1, nullptr);
		m_sb_buffer_.Create(cmd->GetList(), 1, nullptr);
		cmd->FlagReady();

		SetCount(1);
		SetSize(1.f);
	}

	void ParticleRenderer::Update(const float& dt)
	{
		if (m_cs_ && GetMaterial().lock())
		{
			const auto& cmd  = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Particle Renderer").lock();
			const auto& heap = Managers::RenderPipeline::GetInstance().AcquireHeapSlot().lock();

			cmd->SoftReset();

			cmd->GetList()->SetComputeRootSignature(Managers::RenderPipeline::GetInstance().GetRootSignature());
			cmd->GetList()->SetPipelineState(m_cs_->GetPipelineState());

			CheckSize<UINT>(m_instances_.size(), L"Warning: Particle instance size is too much for structured buffer!");

			m_sb_buffer_.SetData(cmd->GetList(), static_cast<UINT>(m_instances_.size()), m_instances_.data());
			m_sb_buffer_.TransitionToUAV(cmd->GetList());
			m_sb_buffer_.CopyUAVHeap(heap);

			const auto thread      = m_cs_->GetThread();
			const auto flatten     = thread[0] * thread[1] * thread[2];
			const UINT group_count = static_cast<UINT>(m_instances_.size() / flatten);
			const UINT remainder   = static_cast<UINT>(m_instances_.size() % flatten);

			m_cs_->SetGroup({group_count + (remainder ? 1 : 0), 1, 1});
			m_cs_->Dispatch(cmd->GetList(), heap, m_params_, m_local_param_buffer_);

			m_sb_buffer_.TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			cmd->FlagReady
					(
					 [this]()
					 {
						 std::lock_guard<std::mutex> lock(m_instances_mutex_);
						 CheckSize<UINT>(m_instances_.size(), L"Warning: Instance size for particle renderer is too large!");
						 m_sb_buffer_.GetData(static_cast<UINT>(m_instances_.size()), m_instances_.data());

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
					);

			heap->Release();
		}
	}

	void ParticleRenderer::PreUpdate(const float& dt) {}

	void ParticleRenderer::FixedUpdate(const float& dt) {}

	void ParticleRenderer::OnSerialized()
	{
		RenderComponent::OnSerialized();
	}

	void ParticleRenderer::OnDeserialized()
	{
		RenderComponent::OnDeserialized();

		if (const auto cs = Resources::ComputeShader::GetByMetadataPath(m_cs_meta_path_str_).lock())
		{
			m_cs_ = cs;
		}
	}

	aligned_vector<Graphics::SBs::InstanceSB> ParticleRenderer::GetParticles()
	{
		std::lock_guard<std::mutex> lock(m_instances_mutex_);
		return reinterpret_cast<aligned_vector<Graphics::SBs::InstanceSB>&>(m_instances_);
	}

	void ParticleRenderer::SetFollowOwner(const bool follow)
	{
		m_b_follow_owner_ = follow;
	}

	void ParticleRenderer::SetCount(const size_t count)
	{
		std::lock_guard<std::mutex> lock(m_instances_mutex_);
		// Expand and apply the world matrix of the owner to each instance.
		m_instances_.resize(count);
		m_params_.SetParam(particle_count_slot, static_cast<int>(count));
	}

	void ParticleRenderer::SetDuration(const float duration)
	{
		std::lock_guard<std::mutex> lock(m_instances_mutex_);
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
		: RenderComponent(RENDER_COM_T_PARTICLE, {}),
		  m_b_follow_owner_(true) {}

	void ParticleRenderer::SetComputeShader(const Weak<Resources::ComputeShader>& cs)
	{
		if (const auto shader = cs.lock())
		{
			m_cs_               = shader;
			m_cs_meta_path_str_ = shader->GetMetadataPath().string();
		}
	}
}

Engine::Graphics::SBs::InstanceParticleSB::InstanceParticleSB()
{
	SetLife(0.0f);
	SetActive(true);
	SetVelocity(Vector3::One);
	SetWorld(Matrix::Identity);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetLife(const float life)
{
	SetParam(0, life);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetActive(const bool active)
{
	SetParam(0, static_cast<int>(active));
}

void Engine::Graphics::SBs::InstanceParticleSB::SetVelocity(const Vector3& velocity)
{
	SetParam(0, velocity);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetWorld(const Matrix& world)
{
	SetParam(0, world);
}

Matrix& Engine::Graphics::SBs::InstanceParticleSB::GetWorld()
{
	return GetParam<Matrix>(0);
}

bool& Engine::Graphics::SBs::InstanceParticleSB::GetActive()
{
	return reinterpret_cast<bool&>(GetParam<int>(0));
}
