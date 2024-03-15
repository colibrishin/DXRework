#include "pch.h"
#include "egObserver.h"
#include "egObserverController.h"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Engine::Objects::Observer,
 _ARTAG(_BSTSUPER(Engine::Abstract::ObjectBase))
)

namespace Engine::Objects
{
  OBJ_CLONE_IMPL(Observer)

  Observer::Observer()
    : ObjectBase(DEF_OBJ_T_OBSERVER) {}

  void Observer::Initialize()
  {
    ObjectBase::Initialize();

    const auto tr = AddComponent<Components::Transform>().lock();
    AddComponent<Components::ObserverController>();

    tr->SetLocalPosition({0.f, 0.f, -10.f});
  }

  Observer::~Observer() {}

  void Observer::PreUpdate(const float& dt) { ObjectBase::PreUpdate(dt); }

  void Observer::Update(const float& dt) { ObjectBase::Update(dt); }

  void Observer::PreRender(const float& dt) { ObjectBase::PreRender(dt); }

  void Observer::Render(const float& dt) { ObjectBase::Render(dt); }

  void Observer::PostRender(const float& dt) { ObjectBase::PostRender(dt); }

  void Observer::FixedUpdate(const float& dt) { ObjectBase::FixedUpdate(dt); }

  void Observer::OnImGui() { ObjectBase::OnImGui(); }
} // namespace Engine::Objects
