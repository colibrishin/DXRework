#pragma once
#include "egCommon.hpp"
#include "egStateController.hpp"

namespace Engine::Components
{
  class ObserverController final : public StateController
  {
  public:
    explicit ObserverController(const WeakObject& owner);

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

  protected:
    ObserverController();

    virtual void Mouse(const float& dt);
    virtual void Move(const float& dt);

  private:
    SERIALIZE_DECL
    COMP_CLONE_DECL

  };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::ObserverController);
