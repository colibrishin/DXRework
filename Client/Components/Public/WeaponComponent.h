#pragma once

#include "Component.h"
#include "Client.h"

#include "WeaponComponent.generated.h"

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API WeaponComponent : public Engine::Abstracts::Component
{
    GENERATE_BODY

    void Initialize() override;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Update(const float dt) override;
    void FixedUpdate(const float dt) override;
    void OnSerialized() override;
    void OnDeserialized() override;

    float GetFireRate() const;

    Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

private:
    WeaponComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner);
    WeaponComponent() : Component({}) {};
   
    EPROPERTY()
    float m_fire_rate_;
};