#include "pch.h"
#include "egObserver.h"
#include "egObserverController.h"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Engine::Objects::Observer,
 _ARTAG(_BSTSUPER(Engine::Abstract::Object))
)

namespace Engine::Objects
{
  Observer::Observer()
    : Object(DEF_OBJ_T_OBSERVER) {}

  void Observer::Initialize()
  {
    Object::Initialize();

    const auto tr = AddComponent<Components::Transform>().lock();
    AddComponent<Components::ObserverController>();

    tr->SetLocalPosition({0.f, 0.f, -10.f});
  }

  Observer::~Observer() {}

  void Observer::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

  void Observer::Update(const float& dt) { Object::Update(dt); }

  void Observer::PreRender(const float& dt) { Object::PreRender(dt); }

  void Observer::Render(const float& dt) { Object::Render(dt); }

  void Observer::PostRender(const float& dt) { Object::PostRender(dt); }

  void Observer::FixedUpdate(const float& dt) { Object::FixedUpdate(dt); }

  void Observer::OnImGui() { Object::OnImGui(); }
} // namespace Engine::Objects
