#pragma once
#include "Component.h"
#include "Sound.h"

#include "SoundPlayer.generated.h"

namespace Engine::Components
{
    ECLASS(component, serialize)
    class ENGINE_SOUNDPLAYER_API SoundPlayer : public Engine::Abstracts::Component
    {
        GENERATE_BODY

        void Initialize() override;
        void PreUpdate(const float dt) override;
        void FixedUpdate(const float dt) override;
        void Update(const float dt) override;
        void OnSerialized() override;
        void OnDeserialized() override;

        void SetSound(const Weak<Engine::Resources::Sound>& sound);
        void PlaySound(bool loop);
        void StopSound();

#if WITH_EDITOR
        void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
        eComponentUpdatePriorities GetUpdatePriority() const override;
    private:
        friend struct ConstructorAccess;
        SoundPlayer(const Weak<Engine::Abstracts::ObjectBase>& owner) : Component(owner) {}
        SoundPlayer() : Component({}) {}

        void CheckTransform(const Weak<Component> component);

#if WITH_EDITOR
        bool m_b_sound_dialog_ = false;
#endif

        EPROPERTY()
        std::filesystem::path m_sound_meta_path_;

        bool m_b_playing_ = false;

        Strong<Resources::Sound> m_loaded_sound_;
    };
}