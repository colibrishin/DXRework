#include "pch.h"
#include "egObserverController.h"
#include "egApplication.h"
#include "egCamera.h"
#include "egMouseManager.h"
#include "egObject.hpp"
#include "egSceneManager.hpp"
#include "egStateController.hpp"
#include "egTransform.h"

BOOST_CLASS_EXPORT_IMPLEMENT(
                             Engine::Abstract::StateController<Engine::eObserverState>)

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::ObserverController,
                       _ARTAG(_BSTSUPER(Engine::Abstract::StateController<eObserverState>)))

namespace Engine::Components
{
    ObserverController::ObserverController(const WeakObject& owner): StateController(owner) {}

    void ObserverController::Initialize()
    {
        StateController::Initialize();
    }

    void ObserverController::PreUpdate(const float& dt)
    {
        StateController::PreUpdate(dt);
    }

    void ObserverController::Update(const float& dt)
    {
        Mouse(dt);
        Move(dt);
    }

    void ObserverController::FixedUpdate(const float& dt) {}

    void ObserverController::PostUpdate(const float& dt)
    {
        StateController<eObserverState>::PostUpdate(dt);
    }

    ObserverController::ObserverController(): StateController() {}

    void ObserverController::Mouse(const float& dt)
    {
        if (const auto scene = GetSceneManager().GetActiveScene().lock())
        {
            const auto mouse = GetApplication().GetMouseState();

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
            {
                return;
            }

            if (mouse.leftButton)
            {
                const auto camera = scene->GetMainCamera().lock();

                camera->SetLookAtRotation(GetMouseManager().GetMouseRotation());

                m_offset_ = camera->GetLookAt();
            }
        }
    }

    void ObserverController::Move(const float& dt)
    {
        const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

        const float speed = 2.0f * dt;
        const auto  ortho =
                Vector3::Transform(
                                   m_offset_, Matrix::CreateRotationY(
                                                                      -DirectX::XMConvertToRadians(90.0f))) *
                speed;

        if (GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
        {
            tr->Translate(m_offset_ * speed);
        }

        if (GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
        {
            tr->Translate(ortho);
        }

        if (GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
        {
            tr->Translate(-m_offset_ * speed);
        }

        if (GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
        {
            tr->Translate(-ortho);
        }
    }
} // namespace Engine::Component
