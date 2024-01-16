#include "pch.h"
#include "clCharacterController.hpp"
#include "client.h"

#include <boost/serialization/export.hpp>
#include <egCamera.h>
#include <egRigidbody.h>
#include <egApplication.h>
#include <egDebugger.hpp>
#include <egCollisionDetector.h>
#include <egTransform.h>

#include "clPlayer.h"
#include "egBaseCollider.hpp"
#include "egMouseManager.h"
#include "egSceneManager.hpp"

BOOST_CLASS_EXPORT_IMPLEMENT(
                             Engine::Abstract::StateController<Client::eCharacterState>)

SERIALIZER_ACCESS_IMPL(
                       Client::State::CharacterController,
                       _ARTAG(_BSTSUPER(Engine::Abstract::StateController<eCharacterState>))
                       _ARTAG(m_shoot_interval) _ARTAG(m_hp_))

namespace Client::State
{
    void CharacterController::Initialize()
    {
        SetState(CHAR_STATE_IDLE);
        const auto cam = GetOwner().lock()->GetScene().lock()->GetMainCamera().lock();

        if (cam)
        {
            m_head_ = GetOwner().lock()->GetSharedPtr<Object::Player>()->GetHead().lock();
            m_head_.lock()->AddChild(cam);
        }
    }

    void CharacterController::PreUpdate(const float& dt)
    {
        StateController::PreUpdate(dt);
    }

    void CharacterController::PostUpdate(const float& dt)
    {
        StateController<eCharacterState>::PostUpdate(dt);
    }

    void CharacterController::CheckJump(
        const boost::shared_ptr<Engine::Components::Rigidbody>& rb)
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
        const boost::shared_ptr<Engine::Components::Rigidbody>& rb)
    {
        float      speed   = 1.0f;
        const auto scene   = Engine::GetSceneManager().GetActiveScene().lock();
        const auto forward = m_head_.lock()->GetComponent<Components::Transform>().lock()->Forward();
        const auto ortho   =
                Vector3::Transform(
                                   forward,
                                   Matrix::CreateRotationY(-XMConvertToRadians(90.0f))) *
                speed;
        bool pressed = false;

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
        {
            rb->AddForce(forward);
            pressed = true;
        }

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
        {
            rb->AddForce(ortho);
            pressed = true;
        }

        if (Engine::GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
        {
            rb->AddForce(-forward);
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
            SetState(CHAR_STATE_ATTACK);

            if (m_shoot_interval < 0.5f)
            {
                m_shoot_interval += dt;
                return false;
            }

            m_shoot_interval = 0.f;
            const auto tr    =
                    GetOwner().lock()->GetComponent<Engine::Components::Transform>().lock();

            Ray ray;
            ray.position  = tr->GetWorldPosition();
            ray.direction = tr->Forward();

            constexpr float distance = 5.f;

            Engine::GetDebugger().Draw(ray, Colors::AliceBlue);
            std::vector<WeakObject> out;

            if (Engine::GetCollisionDetector().Hitscan(
                ray.position, distance, ray.direction, out))
            {
                return true;
            }
        }

        return false;
    }

    void CharacterController::CheckGround() const
    {
        const auto scene = GetOwner().lock()->GetScene().lock();
        const auto& tree = scene->GetObjectTree();
        const auto rb = GetOwner().lock()->GetComponent<Engine::Components::Rigidbody>().lock();

        std::queue<const Octree*> q;
        q.push(&tree);

        while (!q.empty())
        {
            const auto node = q.front();
            q.pop();

            const auto& value = node->Read();
            const auto& children = node->Next();

            for (const auto v : value)
            {
                const auto lcl = GetOwner().lock()->GetComponent<Engine::Components::Collider>().lock();
                const auto rcl = v.lock()->GetComponent<Engine::Components::Collider>().lock();

                if (!rcl || lcl == rcl)
                {
                    continue;
                }

                if (Components::Collider::Intersects(lcl, rcl, Vector3::Down))
                {
                    rb->SetGrounded(true);
                    return;
                }
            }

            for (const auto& child : children)
            {
                if (child && 
                    child->Contains(GetOwner().lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition()))
                {
                    q.push(child);
                }
            }
        }
    }

    void CharacterController::Update(const float& dt)
    {
        const auto rb = GetOwner().lock()->GetComponent<Engine::Components::Rigidbody>().lock();

        if (!rb)
        {
            return;
        }

        const auto head_tr = m_head_.lock()->GetComponent<Components::Transform>().lock();
        const auto body_tr = GetOwner().lock()->GetComponent<Components::Transform>().lock();
        const auto mouse_x = Engine::GetMouseManager().GetMouseXRotation();
        const auto mouse_y = Engine::GetMouseManager().GetMouseYRotation();

        body_tr->SetLocalRotation(mouse_x);
        head_tr->SetLocalRotation(mouse_y);

        CheckGround();
        CheckJump(rb);
        CheckMove(rb);
        CheckAttack(dt);

        switch (GetState())
        {
        case CHAR_STATE_IDLE: if (HasStateChanged()) Engine::GetDebugger().Log("Idle");
            break;
        case CHAR_STATE_WALK: if (HasStateChanged()) Engine::GetDebugger().Log("Walk");
            break;
        case CHAR_STATE_RUN: break;
        case CHAR_STATE_JUMP: if (HasStateChanged()) Engine::GetDebugger().Log("Jump");
            break;
        case CHAR_STATE_ATTACK: if (HasStateChanged()) Engine::GetDebugger().Log("Attack");
            break;
        case CHAR_STATE_DIE: break;
        case CHAR_STATE_HIT: break;
        case CHAR_STATE_MAX:
        default: break;
        }
    }

    void CharacterController::FixedUpdate(const float& dt) {}
} // namespace Client::State
