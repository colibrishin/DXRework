#include "pch.h"
#include "egObserver.h"
#include "egCollider.hpp"
#include "egObserverController.h"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::Observer,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Engine::Objects
{
    void Observer::Initialize()
    {
        Object::Initialize();

        AddComponent<Components::Transform>();
        AddComponent<Components::ObserverController>();
    }

    Observer::~Observer() {}

    void Observer::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Observer::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Observer::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void Observer::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void Observer::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Observer::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }

    void Observer::OnImGui()
    {
        Object::OnImGui();
    }

    void Observer::OnDeserialized() {}
} // namespace Engine::Objects
