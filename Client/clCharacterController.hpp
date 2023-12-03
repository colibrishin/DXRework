#pragma once

#include "clGiftboxObject.hpp"
#include "Client.h"
#include "egStateController.hpp"

namespace Client::State
{
	class CharacterController : public Engine::Abstract::StateController<eCharacterState>
	{
	public:
		explicit CharacterController(const Engine::WeakObject& owner)
			: StateController<eCharacterState>(owner), m_shoot_interval(0.3f), m_hp_(100.f)
		{
		}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

	protected:
		CharacterController() : StateController<eCharacterState>(), m_shoot_interval(0.3f), m_hp_(100.f)
		{
		}

	private:
		SERIALIZER_ACCESS

		void CheckJump(const boost::shared_ptr<Engine::Component::Rigidbody>& rb);
		void CheckMove(const boost::shared_ptr<Engine::Component::Rigidbody>& rb);
		bool CheckAttack(const float& dt);

		Vector3 m_offset_;
		float m_shoot_interval;
		float m_hp_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::StateController<Client::eCharacterState>)
BOOST_CLASS_EXPORT_KEY(Client::State::CharacterController)