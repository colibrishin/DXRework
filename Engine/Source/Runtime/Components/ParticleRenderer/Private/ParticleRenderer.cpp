#include "../Public/ParticleRenderer.h"

#include "ParticleComputeShader.h"
#include "ParticleRenderer.generated.h"



#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"
#include "UIHelpersResourceManager.h"

#include "Components/Transform/Public/Transform.h"

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
		GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
		m_sb_buffer_ = std::make_unique<decltype(m_sb_buffer_)::element_type>( gi.GetStructuredBuffer<Graphics::SBs::InstanceParticleSB>() );
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
			m_cs_->Dispatch(&primitive, groups, m_params_, dt);
			m_sb_buffer_->TransitionCommon(&primitive);
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

            UIInterface &ui = UIInterfaceAccessor::GetInterface();

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
            *parent |= ui.NewCheckbox( this, "FollowOwner", { "Follow Owner", m_b_follow_owner_ } );
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
		m_params_.SetParam(particle_count_slot, static_cast<int>(m_count_));

		for (auto& instance : m_instances_)
		{
			instance.SetActive(true);
			instance.SetLife(m_duration_);
		}
	}

	void ParticleRenderer::SetDuration(const float duration)
	{
		std::lock_guard lock(m_instances_mutex_);
		m_duration_ = duration;
		m_params_.SetParam(duration_slot, m_duration_);

		for (auto& instance : m_instances_)
		{
			instance.SetLife(m_duration_);
		}
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
}
