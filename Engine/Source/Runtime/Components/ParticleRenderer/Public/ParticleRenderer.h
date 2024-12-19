#pragma once
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/StructuredBuffer.h"

#if defined(USE_DX12)
#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferDX12.hpp"
#endif

namespace Engine
{
	struct ParticleRendererExtension;

	namespace Graphics::SBs
	{
		struct PARTICLERENDERER_API InstanceParticleSB : public InstanceSB
		{
		public:
			InstanceParticleSB();
			void SetLife(const float life);
			void SetActive(const bool active);
			void SetVelocity(const Vector3& velocity);
			void SetWorld(const Matrix& world);
			Matrix& GetWorld();
			bool& GetActive();
		};
	}

	using InstanceParticles = aligned_vector<Graphics::SBs::InstanceParticleSB>;
}

namespace Engine::Components
{
	class PARTICLERENDERER_API ParticleRenderer : public RenderComponent
	{
	public:
		// int
		constexpr static size_t particle_count_slot = 0;

		// float
		constexpr static size_t duration_slot = 0;
		constexpr static size_t size_slot     = 1;

		RENDER_COM_T(RENDER_COM_T_PARTICLE)

		ParticleRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);

		ParticleRenderer(const ParticleRenderer& other);
		ParticleRenderer& operator=(const ParticleRenderer& other);

		ParticleRenderer(ParticleRenderer&& other) noexcept            = delete;
		ParticleRenderer& operator=(ParticleRenderer&& other) noexcept = delete;

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] aligned_vector<Graphics::SBs::InstanceSB> GetParticles();

		void SetFollowOwner(bool follow);
		void SetCount(size_t count);
		void SetDuration(float duration);
		void SetSize(float size);
		void SetComputeShader(const Weak<Resources::ComputeShader>& cs);

		bool IsFollowOwner() const;

	private:
		COMP_CLONE_DECL

		friend class Resources::ComputeShader;
		friend struct Engine::ParticleRendererExtension;
		ParticleRenderer();

		bool m_b_follow_owner_;

		Graphics::SBs::LocalParamSB                                   m_params_;
		Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>       m_local_param_buffer_;
		Graphics::StructuredBuffer<Graphics::SBs::InstanceParticleSB> m_sb_buffer_;

		std::mutex        m_instances_mutex_;
		InstanceParticles m_instances_;

		std::string m_cs_meta_path_str_;
		// Note that we need to store in strong sense due to the gc by the resource manager.
		Strong<Resources::ComputeShader> m_cs_;
	};
}
