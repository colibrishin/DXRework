#pragma once
#include "egCommon.hpp"
#include "egComponent.h"

namespace Engine::Components
{
	class SoundPlayer final : public Abstract::Component
	{
	public:
		COMPONENT_T(COM_T_SOUND_PLAYER)

		SoundPlayer(const WeakObjectBase& owner);
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void SetSound(const StrongSound& sound);

		void OnSerialized() override;
		void OnDeserialized() override;
		void OnImGui() override;

		void PlaySound();
		void PlaySoundLoop();

		void StopSound();

	private:
		SERIALIZE_DECL
		COMP_CLONE_DECL
		SoundPlayer();

		std::string m_sound_meta_path_;

		// non-serialized
		StrongSound m_sound_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::SoundPlayer)
