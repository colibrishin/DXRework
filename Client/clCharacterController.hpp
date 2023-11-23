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
			: StateController<eCharacterState>(owner), m_shoot_interval(0.f), m_hp_(100.f)
		{
		}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

	private:
		void CheckJump(const std::shared_ptr<Engine::Component::Rigidbody>& rb);
		void CheckMove(const std::shared_ptr<Engine::Component::Rigidbody>& rb);
		bool CheckAttack(const float& dt);

		Vector3 m_offset_;
		float m_shoot_interval;
		float m_hp_;

	};

	inline void CharacterController::Initialize()
	{
		SetState(CHAR_STATE_IDLE);
	}

	inline void CharacterController::PreUpdate(const float& dt)
	{
		StateController::PreUpdate(dt);
	}

	inline void CharacterController::CheckJump(const std::shared_ptr<Engine::Component::Rigidbody>& rb)
	{
		if (!rb->GetGrounded())
		{
			SetState(CHAR_STATE_JUMP);
		}
		else
		{
			SetState(CHAR_STATE_IDLE);
		}
	}

	inline void CharacterController::CheckMove(const std::shared_ptr<Engine::Component::Rigidbody>& rb)
	{
		float speed = 1.0f;
		const auto ortho = XMVector3Orthogonal(m_offset_);
		bool pressed = false;

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
		{
			rb->AddForce(m_offset_);
			pressed = true;
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
		{
			rb->AddForce(-ortho);
			pressed = true;
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
		{
			rb->AddForce(-m_offset_);
			pressed = true;
		}

		if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
		{
			rb->AddForce(ortho);
			pressed = true;
		}

		if (!pressed)
		{
			SetState(CHAR_STATE_IDLE);
		}
		else
		{
			SetState(CHAR_STATE_WALK);
		}
	}

	inline bool CharacterController::CheckAttack(const float& dt)
	{
		if (Engine::GetApplication().GetMouseState().leftButton)
		{
			if (m_shoot_interval < 0.5f)
			{
				m_shoot_interval += dt;
				return false;
			}

			m_shoot_interval = 0.f;
			const auto tr = GetOwner().lock()->GetComponent<Engine::Component::Transform>().lock();
			std::set<Engine::WeakObject, Engine::WeakObjComparer> out;

			Ray ray;
			ray.position = tr->GetPosition();
			ray.direction = m_offset_;

			constexpr float distance = 5.f;

			if (Engine::GetCollisionDetector().Hitscan(ray, distance, out))
			{
				SetState(CHAR_STATE_ATTACK);
				return true;
			}
		}

		return false;
	}

	inline void CharacterController::Update(const float& dt)
	{
		const auto rb = GetOwner().lock()->GetComponent<Engine::Component::Rigidbody>().lock();

		if (!rb)
		{
			return;
		}

		const auto lookAt = Engine::GetSceneManager().GetActiveScene().lock()->GetMainCamera().lock()->GetLookAtVector();
		m_offset_ = lookAt * Vector3{1.f, 0.f, 1.f};

		CheckJump(rb);
		CheckMove(rb);
		CheckAttack(dt);

		switch (GetState())
		{
		case CHAR_STATE_IDLE:
			if (HasStateChanged())
				Engine::GetDebugger().Log(L"Idle");
			break;
		case CHAR_STATE_WALK:
			if (HasStateChanged())
				Engine::GetDebugger().Log(L"Walk");
			break;
		case CHAR_STATE_RUN:
			break;
		case CHAR_STATE_JUMP:
			if (HasStateChanged())
				Engine::GetDebugger().Log(L"Jump");
			break;
		case CHAR_STATE_ATTACK:
			if (HasStateChanged())
				Engine::GetDebugger().Log(L"Attack");
			break;
		case CHAR_STATE_DIE:
			break;
		case CHAR_STATE_HIT:
			break;
		case CHAR_STATE_MAX:
		default:
			break;
		}
	}

	inline void CharacterController::FixedUpdate(const float& dt)
	{
	}

	inline void CharacterController::PreRender(const float dt)
	{
	}

	inline void CharacterController::Render(const float dt)
	{
	}
}
