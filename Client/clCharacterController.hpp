#pragma once

#include "Client.h"
#include "egObject.hpp"
#include "egStateController.hpp"

namespace Client::State
{
  class CharacterController : public Components::StateController
  {
  public:
    explicit CharacterController(const WeakObject& owner)
      : StateController(owner),
        m_top_view_(false),
        m_cam_id_(g_invalid_id),
        m_shoot_interval(0.3f),
        m_hp_(100.f) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void OnImGui() override;

    void SetActive(bool active) override;
    void MoveCameraToChild(bool active);
    void SetHeadView(const bool head_view);

    void Hit(const float damage);
    void Hitscan(const float damage, const float range) const;

    UINT GetHealth() const;

  protected:
    CharacterController()
      : StateController({}),
        m_top_view_(false),
        m_cam_id_(g_invalid_id),
        m_shoot_interval(0.3f),
        m_hp_(100.f) {}

  private:
    SERIALIZER_ACCESS

    void CheckJump(const boost::shared_ptr<Components::Rigidbody>& rb);
    void CheckMove(const boost::shared_ptr<Components::Rigidbody>& rb);
    void CheckAttack(const float& dt);
    void CheckGround() const;

    bool         m_top_view_;
    WeakObject   m_head_;
    LocalActorID m_cam_id_;
    float        m_shoot_interval;
    float        m_hp_;
  };
} // namespace Client::State

BOOST_CLASS_EXPORT_KEY(Client::State::CharacterController)
