#pragma once
#include "Client.h"
#include <egScript.h>

namespace Client::Scripts
{
  class PlayerHitboxScript : public Engine::Script
  {
  public:
	  CLIENT_SCRIPT_T(PlayerHitboxScript, SCRIPT_T_PLAYER_HITBOX)

	  PlayerHitboxScript(const WeakObjectBase& owner);

	  void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override; 
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

    void OnDeserialized() override;
    void OnSerialized() override;
    void OnImGui() override;

    WeakObjectBase GetHead() const;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    PlayerHitboxScript();

    void updateHitbox() const;

  };
} // namespace Client::Scripts

BOOST_CLASS_EXPORT_KEY(Client::Scripts::PlayerHitboxScript)