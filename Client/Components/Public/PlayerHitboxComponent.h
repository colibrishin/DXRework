#pragma once

#include "Component/Public/Component.h"

#include "PlayerHitboxComponent.generated.h"

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API PlayerHitboxComponent : public Engine::Abstracts::Component
{
    GENERATE_BODY

    void Initialize() override;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Update(const float dt) override;
    void FixedUpdate(const float dt) override;
    void OnSerialized() override;
    void OnDeserialized() override;
    Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

    Engine::Weak<Engine::Abstracts::ObjectBase> GetHead() const;

private:
    PlayerHitboxComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner) : Component(owner) {};
    PlayerHitboxComponent() : Component({}) {};

    void updateHitBox() const;

    void onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other);
    void onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other);
    void onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other);
};