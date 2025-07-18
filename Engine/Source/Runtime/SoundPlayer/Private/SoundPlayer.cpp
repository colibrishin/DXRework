#include "Public/SoundPlayer.h"
#include "SoundPlayer.generated.h"
#include "ObjectBase.h"
#include "Transform.h"
#include "Rigidbody.h"

#if WITH_EDITOR
#include "UIHelpersResourceManager.h"
#endif

void Engine::Components::SoundPlayer::Initialize()
{
    Component::Initialize();

    if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
    {
        owner->onComponentRemoved.Listen(GetWeakPtr<SoundPlayer>(), &SoundPlayer::CheckTransform);

        if (owner->GetComponent<Transform>().expired())
        {
            owner->AddComponent<Transform>();
        }
    }
}

void Engine::Components::SoundPlayer::Update(const float dt)
{
    if (m_b_playing_)
    {
        if (m_loaded_sound_)
        {
            if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
            {
                Vector3 velocity{};
                if (const Strong<Rigidbody>& rb = owner->GetComponent<Rigidbody>().lock())
                {
                    velocity = rb->GetT0LinearVelocity();
                }

                m_loaded_sound_->UpdatePositionAndVelocity(owner->GetComponent<Transform>(), velocity);
            }
        }
    }
}

void Engine::Components::SoundPlayer::OnSerialized()
{
    if ( m_loaded_sound_ )
    {
        Serializer::Serialize(m_loaded_sound_->GetName(), m_loaded_sound_);
        m_sound_meta_path_ = m_loaded_sound_->GetMetadataPath();
    }
}

void Engine::Components::SoundPlayer::OnDeserialized()
{
    if ( const Strong<Resources::Sound>& sound = Resources::Sound::GetByMetadataPath( m_sound_meta_path_ ).lock() )
    {
        SetSound(sound);
    }
}

void Engine::Components::SoundPlayer::SetSound(const Weak<Engine::Resources::Sound>& sound)
{
    if (const Strong<Resources::Sound>& locked = sound.lock())
    {
        m_loaded_sound_ = locked;
        m_sound_meta_path_ = locked->GetMetadataPath();
    }
}

void Engine::Components::SoundPlayer::PlaySound(bool loop)
{
    if (m_loaded_sound_)
    {
        if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
        {
            Vector3 velocity{};
            if (const Strong<Rigidbody>& rb = owner->GetComponent<Rigidbody>().lock())
            {
                velocity = rb->GetT0LinearVelocity();
            }

            m_loaded_sound_->Play(owner->GetComponent<Transform>(), velocity, loop);
            m_b_playing_ = true;
        }
    }
}

void Engine::Components::SoundPlayer::StopSound()
{
    if (m_loaded_sound_)
    {
        if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
        {
            m_loaded_sound_->Stop(owner->GetComponent<Transform>());
            m_b_playing_ = false;
        }
    }
}

#if WITH_EDITOR
void Engine::Components::SoundPlayer::OnUIUpdate(UIContext* const parent, const float dt)
{
    if ( parent )
    {
        Component::OnUIUpdate( parent, dt );
        {
            IUIAPI&       ui = g_ui_accessor.GetInterface();
            static std::string sound_name;
            if (m_loaded_sound_)
            {
                sound_name = m_loaded_sound_->GetName();
            }
            else 
            {
                sound_name = "";
            }
            *parent |= ui.NewLabelAndText( this, "Sound", { "Sound", sound_name, false } );
            ( *parent |= ui.NewButton( this, "SoundSelectButton", { "Select Sound..." } ) ).SetFunction( [this]()
            {
                m_b_sound_dialog_ = !m_b_sound_dialog_;
            } );
        }

        if (m_b_sound_dialog_) 
        {
            if (Weak<Engine::Abstracts::Resource> resource_to_load;
                UIHelpers::SingleResourceSelectionDialogInclusion<SoundPlayer, Resources::Sound>(
                    GetSharedPtr<SoundPlayer>(),
                    resource_to_load))
            {
                if (const Strong<Resources::Sound>& sound = Cast<Resources::Sound>(resource_to_load))
                {
                    SetSound(sound);
                }

                m_b_sound_dialog_ = false;
            }
        }
    }
}
#endif

Engine::eComponentUpdatePriorities Engine::Components::SoundPlayer::GetUpdatePriority() const
{
    return eComponentUpdatePriority::COM_PRIORITY_POSITIONAL | eComponentUpdatePriority::COM_PRIORITY_PHYSICS;
}

void Engine::Components::SoundPlayer::CheckTransform(const Weak<Component> component)
{
    if (const Strong<Component>& locked = component.lock())
    {
        if (locked->IsDerivedOf(Transform::StaticTypeHash()))
        {
            if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
            {
                owner->RemoveComponent<SoundPlayer>();
            }
        }
    }
}

void Engine::Components::SoundPlayer::PreUpdate(const float dt)
{
}

void Engine::Components::SoundPlayer::FixedUpdate(const float dt)
{
}
