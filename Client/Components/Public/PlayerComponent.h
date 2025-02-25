#pragma once

#include "Component.h"
#include "Client.h"

#include "PlayerComponent.generated.h"

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API PlayerComponent : public Engine::Abstracts::Component
{
    GENERATE_BODY

    void Initialize() override;
    void PreUpdate(const float dt) override;
    void PostUpdate(const float dt) override;
    void Update(const float dt) override;
    void FixedUpdate(const float dt) override;
    void OnSerialized() override;
    void OnDeserialized() override;

    void SetActive(bool active) override;

    UINT GetHealth() const;
    void Hit(float damage);

    Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

private:
    PlayerComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner);
    PlayerComponent() : Component({}) {};
    
    [[nodiscard]] Engine::Weak<Engine::Abstracts::ObjectBase> getHead() const;
    [[nodiscard]] float getFireRate() const;

    void hitScan(float damage, float range) const;

    eCharacterState getState() const;
    void setState(eCharacterState state);
    bool hasStateChanged() const;

    void moveCameraToChild(bool active);
    void setHeadView(bool head_view);

    void checkJump(const Engine::Strong<Engine::Components::Rigidbody>& rb);
    void checkMove(const Engine::Strong<Engine::Components::Rigidbody>& rb);
    void checkAttack(const float dt);
    void checkGround();

    void onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other);
    void onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other);
    void onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other);

    EPROPERTY()
    eCharacterState m_state_;
    EPROPERTY()
    eCharacterState m_prev_state_;
    EPROPERTY()
    bool m_top_view_;
    EPROPERTY()
    Engine::LocalActorID m_cam_id_;
    EPROPERTY()
    float m_hp_;
    EPROPERTY()
    float m_fire_interval_;
    EPROPERTY()
    bool m_b_grounded_;

};