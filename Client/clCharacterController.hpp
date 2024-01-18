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
        m_shoot_interval(0.3f),
        m_hp_(100.f) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;

  protected:
    CharacterController()
      : StateController({}),
        m_shoot_interval(0.3f),
        m_hp_(100.f) {}

  private:
    SERIALIZER_ACCESS

    void CheckJump(const boost::shared_ptr<Components::Rigidbody>& rb);
    void CheckMove(const boost::shared_ptr<Components::Rigidbody>& rb);
    bool CheckAttack(const float& dt);
    void CheckGround() const;

    WeakObject m_head_;
    float      m_shoot_interval;
    float      m_hp_;
  };
} // namespace Client::State

BOOST_CLASS_EXPORT_KEY(Client::State::CharacterController)
