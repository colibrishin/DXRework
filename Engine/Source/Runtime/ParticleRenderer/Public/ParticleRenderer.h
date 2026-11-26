#pragma once
#include "InstanceParticleSB.h"
#include "ParticleComputeShader.h"
#include "ShapeRenderComponent.h"
#include "Allocator.h"
#include "StructuredBuffer.h"

#include <deque>

#include "ParticleRenderer.generated.h"

namespace Engine::Components
{
	ECLASS(serialize, component)
	class ENGINE_PARTICLERENDERER_API ParticleRenderer : public ShapeRenderComponent
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

		void Initialize() override;
		void Update(const float dt) override;
		void PreUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		eComponentUpdatePriorities GetUpdatePriority() const override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		[[nodiscard]] aligned_vector<Graphics::SBs::InstanceSB> GetParticles();

		void SetFollowOwner(bool follow);
		void SetCount(size_t count);
		void SetDuration(float duration);
		void SetSize(float size);
		void SetComputeShader(const Weak<Resources::ParticleComputeShader>& cs);

		// The CPU copy of the particles: the latest finished simulation step (render task, editor). Code writing it calls
		// MarkInstancesChanged so it is uploaded; the editor's writes are found by comparing.
		InstanceParticles& GetInstances();
		void MarkInstancesChanged();

		bool IsFollowOwner() const;

	private:
		friend class Resources::ComputeShader;
		friend struct ParticleRendererExtension;
		ParticleRenderer();

		EPROPERTY()
		bool m_b_follow_owner_;
		EPROPERTY()
		size_t m_count_;
		EPROPERTY()
		float m_duration_;
		EPROPERTY()
		float m_size_;
		
		EPROPERTY()
		Graphics::SBs::LocalParamSB                                      m_params_;
		Unique<StructuredBufferTypeProxy<Graphics::SBs::InstanceParticleSB>> m_sb_buffer_;

		std::mutex        m_instances_mutex_;
		InstanceParticles m_instances_;

		// The GPU buffer holds the simulation. The CPU copy is uploaded when it changed and otherwise refreshed from the
		// newest finished readback of the current upload, one or two frames late (KOLIBRI_PARTICLE_SYNC=1: upload,
		// simulate and read back every frame, waiting for the GPU, as before).
		struct PendingReadback
		{
			uint64_t tag;
			uint64_t generation;
			size_t   count;
		};
		bool                        m_instances_dirty_ = true;
		uint64_t                    m_generation_      = 0; // uploads so far: readbacks of an earlier one are dropped
		uint64_t                    m_readback_tag_    = 0;
		size_t                      m_active_          = 0; // active particles in the CPU copy (none: nothing to simulate)
		std::deque<PendingReadback> m_readbacks_;
#if WITH_EDITOR
		bool              m_ui_seen_ = false;
		bool              m_ui_snapshot_valid_ = false;
		InstanceParticles m_ui_snapshot_; // the CPU copy as this component last wrote it
#endif

		EPROPERTY()
		std::filesystem::path m_cs_meta_path_;

		Strong<Resources::ParticleComputeShader> m_cs_;
		
		Weak<Resources::ParticleComputeShader> m_cached_cs_;

#if WITH_EDITOR
		bool m_particle_shader_dialog_opened_ = false;
#endif
	};
}
