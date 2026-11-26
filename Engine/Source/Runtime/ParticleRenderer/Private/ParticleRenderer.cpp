#include "ParticleRenderer.h"

#include "ParticleComputeShader.h"

#include "RenderPipeline.h"
#include "ComputeShader.h"
#include "UIHelpersResourceManager.h"

#include "Transform.h"

#include <cstring>

namespace Engine::Components
{
	ParticleRenderer::ParticleRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: ShapeRenderComponent(owner),
		  m_b_follow_owner_(true) {}

	ParticleRenderer::ParticleRenderer(const ParticleRenderer& other)
		: ShapeRenderComponent(other)
	{
		m_cs_             = other.m_cs_;
		m_cs_meta_path_   = other.m_cs_meta_path_;
		m_instances_      = other.m_instances_;
		m_b_follow_owner_ = other.m_b_follow_owner_;
	}

	ParticleRenderer& ParticleRenderer::operator=(const ParticleRenderer& other)
	{
		if (this != &other)
		{
			RenderComponent::operator=(other);
			m_cs_               = other.m_cs_;
			m_cs_meta_path_ = other.m_cs_meta_path_;
			m_instances_        = other.m_instances_;
			m_instances_dirty_  = true;
			m_b_follow_owner_   = other.m_b_follow_owner_;
		}
		return *this;
	}

	void ParticleRenderer::Initialize()
	{
		RenderComponent::Initialize();
		SetCount( 1 );
		SetDuration( 1.f );
		SetSize( 1.f );
		IGraphicAPI& gi = g_graphic_accessor.GetInterface();
		m_sb_buffer_ = std::make_unique<decltype(m_sb_buffer_)::element_type>( gi.GetStructuredBuffer<Graphics::SBs::InstanceParticleSB>() );
	}

	namespace
	{
		size_t CountActive(InstanceParticles& instances)
		{
			size_t active = 0;
			for (auto& instance : instances)
			{
				active += instance.GetActive() ? 1 : 0;
			}
			return active;
		}
	}

	void ParticleRenderer::Update(const float dt)
	{
		if (!m_cs_ || !GetShape().lock())
		{
#if WITH_EDITOR
			m_ui_seen_           = false;
			m_ui_snapshot_valid_ = false;
#endif
			return;
		}

		IGraphicAPI& gi = g_graphic_accessor.GetInterface();
		static const bool sync = [] { const char* v = std::getenv("KOLIBRI_PARTICLE_SYNC"); return v && v[0] == '1'; }();
		if (sync)
		{
			const IGraphicContextImpl& context = gi.GetNewContext(0, true, L"Particle Renderer Update");
			const IGraphicContext& primitive = context.GetPointers();
			
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
			m_cs_->Dispatch(&primitive, groups, m_params_, dt);
			m_sb_buffer_->TransitionCommon(&primitive);
			primitive.commandList->Execute();

			const IGraphicContextImpl& copy_context = gi.GetNewContext(0, true, L"Particle Renderer Update");
			const IGraphicContext& copy_primitive = copy_context.GetPointers();
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
			return;
		}

		std::lock_guard lock(m_instances_mutex_);
#if WITH_EDITOR
		// The editor writes the CPU copy directly (Linear Spread): upload it when it differs from what this component
		// last put there, before a readback would overwrite it.
		if (m_ui_seen_ && m_ui_snapshot_valid_ &&
			(m_ui_snapshot_.size() != m_instances_.size() ||
			 std::memcmp(m_ui_snapshot_.data(), m_instances_.data(), m_instances_.size() * sizeof(m_instances_[0])) != 0))
		{
			m_instances_dirty_ = true;
		}
#endif

		// The newest finished readback of the current upload becomes the CPU copy (no wait: earlier ones are dropped).
		if (const uint64_t tag = m_sb_buffer_->ReadyData())
		{
			while (!m_readbacks_.empty() && m_readbacks_.front().tag < tag)
			{
				m_readbacks_.pop_front();
			}
			if (!m_readbacks_.empty() && m_readbacks_.front().tag == tag)
			{
				const PendingReadback done = m_readbacks_.front();
				m_readbacks_.pop_front();
				if (!m_instances_dirty_ && done.generation == m_generation_ && done.count == m_instances_.size())
				{
					m_sb_buffer_->ReadData(tag, static_cast<UINT>(done.count), m_instances_.data());
					m_active_ = CountActive(m_instances_);
				}
			}
		}

		// Once every particle retired (the newest readback says so), there is nothing to simulate until an upload.
		const bool upload   = m_instances_dirty_;
		const bool simulate = dt > 0.f && (upload ? CountActive(m_instances_) : m_active_) > 0;
		if (upload || simulate)
		{
			const IGraphicContextImpl& context   = gi.GetNewContext(0, true, L"Particle Renderer Update");
			const IGraphicContext&     primitive = context.GetPointers();
			primitive.commandList->SoftReset();

			if (upload)
			{
				CheckSize<UINT>(m_instances_.size(), L"Warning: Particle instance size is too much for structured buffer!");
				m_sb_buffer_->SetData(&primitive, static_cast<UINT>(m_instances_.size()), m_instances_.data());
				m_generation_ += 1;
				m_instances_dirty_ = false;
				m_active_          = CountActive(m_instances_);
			}

			if (simulate)
			{
				m_sb_buffer_->TransitionToUAV(&primitive);
				m_sb_buffer_->CopyUAVHeap(&primitive);

				const auto thread      = m_cs_->GetThread();
				const auto flatten     = thread[0] * thread[1] * thread[2];
				const UINT group_count = static_cast<UINT>(m_instances_.size() / flatten);
				const UINT remainder   = static_cast<UINT>(m_instances_.size() % flatten);

				const UINT groups[3] = {group_count + (remainder ? 1 : 0), 1, 1};
				m_cs_->Dispatch(&primitive, groups, m_params_, dt);
				m_sb_buffer_->TransitionCommon(&primitive);

				m_readback_tag_ += 1;
				if (m_sb_buffer_->RequestData(&primitive, static_cast<UINT>(m_instances_.size()), m_readback_tag_))
				{
					m_readbacks_.push_back({ m_readback_tag_, m_generation_, m_instances_.size() });
				}
			}

			primitive.commandList->FlagReady();
		}

#if WITH_EDITOR
		if (m_ui_seen_)
		{
			m_ui_snapshot_       = m_instances_;
			m_ui_snapshot_valid_ = true;
		}
		else
		{
			m_ui_snapshot_valid_ = false;
		}
		m_ui_seen_ = false;
#endif
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

		if (const auto cs = Resources::ParticleComputeShader::GetByMetadataPath(m_cs_meta_path_).lock())
		{
			m_cs_ = cs;
			m_cached_cs_ = cs;
		}
	}

	eComponentUpdatePriorities ParticleRenderer::GetUpdatePriority() const
	{
		return COM_PRIORITY_RENDER;
	}

#if WITH_EDITOR
    void ParticleRenderer::OnUIUpdate( UIContext *const parent, const float dt )
    {
        if ( parent )
        {
            Base::OnUIUpdate( parent, dt );
            m_ui_seen_ = true; // Update compares the CPU copy with what it last wrote

            IUIAPI &ui = g_ui_accessor.GetInterface();

            static std::string shader_name{};
            if ( const Strong<Resources::ParticleComputeShader> &shader = m_cached_cs_.lock() )
            {
                shader_name = shader->GetName();
            }
            *parent |= ui.NewLabelAndText( this, "ParticleShader", { "Particle Shader", shader_name, false } );
            ( *parent |= ui.NewButton( this, "ParticleShaderButton", { "Set Particle Shader..." } ) ).SetFunction( [&]()
            {
                m_particle_shader_dialog_opened_ = !m_particle_shader_dialog_opened_;
            } );
            *parent |= ui.NewCheckbox( this, "FollowOwner", { "Follow Owner", m_b_follow_owner_, true } );
            ( *parent |= ui.NewLabelAndInt( this,
                                            "ParticleCount",
                                            {
                                                "Particle Count",
                                                m_params_.GetParam<int>( particle_count_slot ),
                                                0,
                                                0,
                                                std::numeric_limits<int>::max(),
                                                true } ) ).SetFunction( [this]()
            {
                SetCount( m_params_.GetParam<int>( particle_count_slot ) );
            } );
            ( *parent |= ui.NewLabelAndFloat( this,
                                              "ParticleDuration",
                                              {
                                                  "Particle Duration",
                                                  m_params_.GetParam<float>( duration_slot ),
                                                  0,
                                                  0.f,
                                                  std::numeric_limits<float>::max(),
                                                  true } ) ).SetFunction( [this]()
            {
                SetDuration( m_params_.GetParam<float>( duration_slot ) );
            } );
            ( *parent |= ui.NewLabelAndInt( this,
                                            "ParticleSize",
                                            {
                                                "Particle Size",
                                                m_params_.GetParam<int>( size_slot ),
                                                0,
                                                0,
                                                std::numeric_limits<int>::max(),
                                                true } ) ).SetFunction( [this]()
            {
                SetCount( m_params_.GetParam<int>( size_slot ) );
            } );

            if ( const Strong<Resources::ParticleComputeShader> &locked = m_cached_cs_.lock() )
            {
                locked->OnUIUpdateParam( parent, dt, m_params_, m_instances_ );
            }

            if ( m_particle_shader_dialog_opened_ )
            {
                if ( Weak<Engine::Abstracts::Resource> resource_to_load;
                    UIHelpers::SingleResourceSelectionDialogInclusion<
                        ParticleRenderer, Resources::ParticleComputeShader>(
                            GetSharedPtr<ParticleRenderer>(),
                            resource_to_load
                            ) )
                {
                    if ( const auto shader = Cast<Resources::ParticleComputeShader>( resource_to_load ) )
                    {
                        SetComputeShader( shader );
                    }

                    m_particle_shader_dialog_opened_ = false;
                }
            }
		}
	}
#endif

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
		m_count_ = count;
		m_instances_.resize(m_count_, {});
		m_instances_dirty_ = true;
		m_params_.SetParam(particle_count_slot, static_cast<int>(m_count_));

		// Life is the time a particle has lived (the shader adds dt and retires it past the duration).
		for (auto& instance : m_instances_)
		{
			instance.SetActive(true);
			instance.SetLife(0.f);
		}
	}

	void ParticleRenderer::SetDuration(const float duration)
	{
		std::lock_guard lock(m_instances_mutex_);
		m_duration_ = duration;
		m_params_.SetParam(duration_slot, m_duration_);
	}

	void ParticleRenderer::SetSize(const float size)
	{
		m_size_ = size;
		m_params_.SetParam(size_slot, m_size_);
	}

	bool ParticleRenderer::IsFollowOwner() const
	{
		return m_b_follow_owner_;
	}

	ParticleRenderer::ParticleRenderer()
		: m_b_follow_owner_(true),
		  m_count_(1),
		  m_duration_(1),
		  m_size_(1) {}

	void ParticleRenderer::SetComputeShader(const Weak<Resources::ParticleComputeShader>& cs)
	{
		if (const auto shader = cs.lock())
		{
			shader->Load();
			m_cs_ = shader;
			m_cached_cs_ = shader;
			m_cs_meta_path_ = shader->GetMetadataPath();
		}
	}

	InstanceParticles& ParticleRenderer::GetInstances()
	{
		return m_instances_;
	}

	void ParticleRenderer::MarkInstancesChanged()
	{
		std::lock_guard lock(m_instances_mutex_);
		m_instances_dirty_ = true;
	}
}
