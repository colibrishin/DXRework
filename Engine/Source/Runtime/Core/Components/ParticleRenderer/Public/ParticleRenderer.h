#pragma once
#include "Source/Runtime/Core/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/StructuredBuffer.h"

#if defined(USE_DX12)
#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferDX12.hpp"
#endif

namespace Engine
{
	namespace Graphics::SBs
	{
		struct InstanceParticleSB : public InstanceSB
		{
		public:
			InstanceParticleSB()
			{
				SetLife(0.0f);
				SetActive(true);
				SetVelocity(Vector3::One);
				SetWorld(Matrix::Identity);
			}

			void SetLife(const float life)
			{
				SetParam(0, life);
			}

			void SetActive(const bool active)
			{
				SetParam(0, static_cast<int>(active));
			}

			void SetVelocity(const Vector3& velocity)
			{
				SetParam(0, velocity);
			}

			void SetWorld(const Matrix& world)
			{
				SetParam(0, world);
			}

			Matrix& GetWorld()
			{
				return GetParam<Matrix>(0);
			}

			bool& GetActive()
			{
				return reinterpret_cast<bool&>(GetParam<int>(0));
			}
		};
	}

	using InstanceParticles = aligned_vector<Graphics::SBs::InstanceParticleSB>;
}

namespace Engine::Components
{
	class ParticleRenderer : public RenderComponent
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
		SERIALIZE_DECL
		COMP_CLONE_DECL

		friend class Resources::ComputeShader;
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

BOOST_CLASS_EXPORT_KEY(Engine::Components::ParticleRenderer)
