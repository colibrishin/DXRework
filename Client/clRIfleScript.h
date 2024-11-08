#pragma once
#include <egScript.h>
#include "Client.h"

namespace Client::Scripts
{
	class RifleScript final : public Script
	{
	public:
		CLIENT_SCRIPT_T(RifleScript, SCRIPT_T_RIFLE)

		RifleScript(const WeakObjectBase& owner);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;

		void OnImGui() override;

		float GetFireRate() const;

	protected:
		void OnCollisionEnter(const WeakCollider& other) override;
		void OnCollisionContinue(const WeakCollider& other) override;
		void OnCollisionExit(const WeakCollider& other) override;

	private:
		SERIALIZE_DECL
		SCRIPT_CLONE_DECL

		RifleScript();

		float m_fire_rate_;
	};
} // namespace Client::Scripts

REGISTER_TYPE(Engine::Script, Client::Scripts::RifleScript)
BOOST_CLASS_EXPORT_KEY(Client::Scripts::RifleScript)
