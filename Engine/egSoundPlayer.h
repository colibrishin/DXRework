#pragma once
#include "egCommon.hpp"
#include "egComponent.h"

namespace Engine::Components
{
    class SoundPlayer final : public Abstract::Component
    {
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COMP_T_SOUND_PLAYER)

        SoundPlayer(const WeakObject& owner);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostRender(const float& dt) override;

        void SetSound(const StrongSound& sound);
        void OnDeserialized() override;

        void PlaySound();
        void PlaySoundLoop();

        void StopSound();

    private:
        SERIALIZER_ACCESS
        SoundPlayer();

        std::string m_sound_name_;

        // non-serialized
        StrongSound m_sound_;

    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::SoundPlayer)