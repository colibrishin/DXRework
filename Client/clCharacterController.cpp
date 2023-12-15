#include "pch.h"
#include "clCharacterController.hpp"
#include "client.h"

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(
                             Engine::Abstract::StateController<Client::eCharacterState>)

SERIALIZER_ACCESS_IMPL(
                       Client::State::CharacterController,
                       _ARTAG(_BSTSUPER(Engine::Abstract::StateController<eCharacterState>))
                       _ARTAG(m_offset_) _ARTAG(m_shoot_interval) _ARTAG(m_hp_))

namespace Client::State
{
    void CharacterController::Initialize()
    {
        SetState(CHAR_STATE_IDLE);
    }

    void CharacterController::PreUpdate(const float& dt)
    {
        StateController::PreUpdate(dt);
    }

    void CharacterController::CheckJump(
        const boost::shared_ptr<Engine::Component::Rigidbody>& rb)
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

    void CharacterController::CheckMove(
        const boost::shared_ptr<Engine::Component::Rigidbody>& rb)
    {
        float      speed = 1.0f;
        const auto ortho =
                Vector3::Transform(
                                   m_offset_,
                                   Matrix::CreateRotationY(-XMConvertToRadians(90.0f))) *
                speed;
        bool pressed = false;

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
        {
            rb->AddForce(m_offset_);
            pressed = true;
        }

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
        {
            rb->AddForce(ortho);
            pressed = true;
        }

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
        {
            rb->AddForce(-m_offset_);
            pressed = true;
        }

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
        {
            rb->AddForce(-ortho);
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

    bool CharacterController::CheckAttack(const float& dt)
    {
        if (Engine::GetApplication().GetMouseState().leftButton)
        {
            if (m_shoot_interval < 0.5f)
            {
                m_shoot_interval += dt;
                return false;
            }

            m_shoot_interval = 0.f;
            const auto tr    =
                    GetOwner().lock()->GetComponent<Engine::Component::Transform>().lock();
            std::set<Engine::WeakObject, Engine::WeakComparer<Engine::Abstract::Object>>
                    out;

            Ray ray;
            ray.position  = tr->GetPosition();
            ray.direction = m_offset_;

            constexpr float distance = 5.f;

            Engine::GetDebugger().Draw(ray, Colors::AliceBlue);

            if (Engine::GetCollisionDetector().Hitscan(ray, distance, out))
            {
                SetState(CHAR_STATE_ATTACK);
                return true;
            }
        }

        return false;
    }

    void CharacterController::Update(const float& dt)
    {
        const auto rb =
                GetOwner().lock()->GetComponent<Engine::Component::Rigidbody>().lock();

        if (!rb)
        {
            return;
        }

        if (const auto scene = Engine::GetSceneManager().GetActiveScene().lock())
        {
            const auto camera = scene->GetMainCamera().lock();

            camera->SetLookAtRotation(Engine::GetMouseManager().GetMouseRotation());

            m_offset_ = camera->GetLookAt() * Vector3{1.f, 0.f, 1.f};

            CheckJump(rb);
            CheckMove(rb);
            CheckAttack(dt);

            switch (GetState())
            {
            case CHAR_STATE_IDLE: if (HasStateChanged()) Engine::GetDebugger().Log(L"Idle");
                break;
            case CHAR_STATE_WALK: if (HasStateChanged()) Engine::GetDebugger().Log(L"Walk");
                break;
            case CHAR_STATE_RUN: break;
            case CHAR_STATE_JUMP: if (HasStateChanged()) Engine::GetDebugger().Log(L"Jump");
                break;
            case CHAR_STATE_ATTACK: if (HasStateChanged()) Engine::GetDebugger().Log(L"Attack");
                break;
            case CHAR_STATE_DIE: break;
            case CHAR_STATE_HIT: break;
            case CHAR_STATE_MAX:
            default: break;
            }
        }
    }

    void CharacterController::FixedUpdate(const float& dt) {}

    void CharacterController::PreRender(const float& dt) {}

    void CharacterController::Render(const float& dt) {}

    void CharacterController::PostRender(const float& dt) {}
} // namespace Client::State
