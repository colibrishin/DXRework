#pragma once

#include "Component/Public/Component.h"

#include "HitboxComponent.generated.h"

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API HitboxComponent : public Engine::Abstracts::Component
{
    GENERATE_BODY

    void Hit(const float dmg) const;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Update(const float dt) override;
    void FixedUpdate(const float dt) override;
    void OnSerialized() override;
    void OnDeserialized() override;

    Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

private:
    using Component::Component;
    HitboxComponent() : Component({}) {}

    EPROPERTY()
    float m_modifier_ = 1.f;

};