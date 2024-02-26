#include "pch.h"
#include "egObserverController.h"
#include "egApplication.h"
#include "egCamera.h"
#include "egMouseManager.h"
#include "egObject.hpp"
#include "egSceneManager.hpp"
#include "egStateController.hpp"
#include "egTransform.h"


SERIALIZER_ACCESS_IMPL
(
 Engine::Components::ObserverController,
 _ARTAG(_BSTSUPER(Engine::Components::StateController))
)

namespace Engine::Components
{
  ObserverController::ObserverController(const WeakObject& owner)
    : StateController(owner) {}

  void ObserverController::Initialize() { StateController::Initialize(); }

  void ObserverController::PreUpdate(const float& dt) { StateController::PreUpdate(dt); }

  void ObserverController::Update(const float& dt)
  {
    Mouse(dt);
    Move(dt);
  }

  void ObserverController::FixedUpdate(const float& dt) {}

  void ObserverController::PostUpdate(const float& dt) { StateController::PostUpdate(dt); }

  void ObserverController::OnSerialized() {}

  void ObserverController::OnDeserialized() { StateController::OnDeserialized(); }

  void ObserverController::OnImGui() { StateController::OnImGui(); }

  ObserverController::ObserverController()
    : StateController({}) {}

  void ObserverController::Mouse(const float& dt)
  {
    const auto mouse = GetApplication().GetMouseState();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) { return; }

    if (mouse.leftButton)
    {
      const auto tr        = GetOwner().lock()->GetComponent<Transform>().lock();
      const auto mouse_rot = GetMouseManager().GetMouseRotation();
      tr->SetLocalRotation(mouse_rot);
    }
  }

  void ObserverController::Move(const float& dt)
  {
    const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

    const float speed   = 2.0f * dt;
    const auto  forward = GetOwner().lock()->GetComponent<Transform>().lock()->Forward();
    const auto  ortho   =
      Vector3::Transform
      (
       forward, Matrix::CreateRotationY
       (
        -DirectX::XMConvertToRadians(90.0f)
       )
      ) *
      speed;

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::W)) { tr->Translate(forward * speed); }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::A)) { tr->Translate(ortho); }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::S)) { tr->Translate(-forward * speed); }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::D)) { tr->Translate(-ortho); }
  }
} // namespace Engine::Component
