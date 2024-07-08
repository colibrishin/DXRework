#pragma once
#include <egScript.h>
#include "Client.h"

namespace Client::Scripts
{
	class PlayerHitboxScript : public Script
	{
	public:
		CLIENT_SCRIPT_T(PlayerHitboxScript, SCRIPT_T_PLAYER_HITBOX)

		PlayerHitboxScript(const WeakObjectBase& owner);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;
		void OnImGui() override;

		WeakObjectBase GetHead() const;

	protected:
		void OnCollisionEnter(const WeakCollider& other) override;
		void OnCollisionContinue(const WeakCollider& other) override;
		void OnCollisionExit(const WeakCollider& other) override;

	private:
		SERIALIZE_DECL
		SCRIPT_CLONE_DECL

		PlayerHitboxScript();

		void updateHitbox() const;
	};
} // namespace Client::Scripts

REGISTER_TYPE(Engine::Script, Client::Scripts::PlayerHitboxScript)
BOOST_CLASS_EXPORT_KEY(Client::Scripts::PlayerHitboxScript)
