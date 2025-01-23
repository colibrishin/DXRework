#pragma once
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/StructuredBuffer/Public/StructuredBuffer.h"
#include "InstanceParticleSB.h"

#include "ParticleRenderer.generated.h"

namespace Engine::Components
{
	ECLASS(serialize)
	class ENGINE_PARTICLERENDERER_API ParticleRenderer : public RenderComponent
	{
		GENERATE_BODY
	public:
		//int
		constexpr static size_t particle_count_slot = 0;

		// float
		constexpr static size_t duration_slot = 0;
		constexpr static size_t size_slot     = 1;

		ParticleRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);

		ParticleRenderer(const ParticleRenderer& other);
		ParticleRenderer& operator=(const ParticleRenderer& other);

		ParticleRenderer(ParticleRenderer&& other) noexcept            = delete;
		ParticleRenderer& operator=(ParticleRenderer&& other) noexcept = delete;

		void Initialize() override;
		void Update(const float dt) override;
		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		eComponentUpdatePriority GetUpdatePriority() const override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

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
		friend struct ParticleRendererExtension;
		ParticleRenderer();

		EPROPERTY()
		bool m_b_follow_owner_;

		EPROPERTY()
		Graphics::SBs::LocalParamSB                                      m_params_;
		Unique<StructuredBufferTypeProxy<Graphics::SBs::InstanceParticleSB>> m_sb_buffer_;

		std::mutex        m_instances_mutex_;
		InstanceParticles m_instances_;

		EPROPERTY()
		std::filesystem::path m_cs_meta_path_;

		Strong<Resources::ComputeShader> m_cs_;
		
		Weak<Resources::ComputeShader> m_cached_cs_;

#if WITH_EDITOR
		bool m_particle_shader_dialog_opened_ = false;
#endif
	};
}
