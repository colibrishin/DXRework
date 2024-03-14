#pragma once
#include "Client.h"
#include "egObject.hpp"
#include "egScript.h"

namespace Client::Scripts
{
  class PlayerScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(PlayerScript, SCRIPT_T_PLAYER)

    explicit PlayerScript(const WeakObject& owner)
      : Script(SCRIPT_T_PLAYER, owner),
        m_state_(CHAR_STATE_IDLE),
        m_prev_state_(CHAR_STATE_IDLE),
        m_bone_initialized_(false),
        m_rifle_initialized_(false),
        m_health_initialized_(false),
        m_top_view_(false),
        m_cam_id_(0),
        m_shoot_interval(0.3f),
        m_hp_(100.f) {}

	  ~PlayerScript() override = default;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override; 
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

    void SetActive(bool active) override;

    void OnDeserialized() override;
    void OnImGui() override;

    UINT GetHealth() const;
    void Hit(const float damage);

  private:
    SERIALIZE_DECL
    PlayerScript();

    void Hitscan(const float damage, const float range) const;

    eCharacterState GetState() const;
    void SetState(const eCharacterState state);
    bool HasStateChanged() const;

    void MoveCameraToChild(bool active);
    void SetHeadView(const bool head_view);

    void CheckJump(const boost::shared_ptr<Components::Rigidbody>& rb);
    void CheckMove(const boost::shared_ptr<Components::Rigidbody>& rb);
    void CheckAttack(const float& dt);
    void CheckGround() const;

    void UpdateHitbox();

    eCharacterState m_state_;
    eCharacterState m_prev_state_;

    bool m_bone_initialized_;
    bool m_rifle_initialized_;
    bool m_health_initialized_;

    std::map<UINT, LocalActorID> m_child_bones_;

    bool         m_top_view_;
    WeakObject   m_head_;
    LocalActorID m_cam_id_;
    float        m_shoot_interval;
    float        m_hp_;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::PlayerScript)