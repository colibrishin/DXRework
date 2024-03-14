#pragma once
#include "Client.h"
#include "egObject.hpp"
#include "egScript.h"
#include <egTransform.h>
#include <egBaseCollider.hpp>

namespace Client::Scripts
{
  class ButtonScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(ButtonScript, SCRIPT_T_BUTTON)

    explicit ButtonScript(const WeakObject& owner)
      : Script(SCRIPT_T_BUTTON, owner),
        m_pressed_(false) {}

	  ~ButtonScript() override = default;

    void Initialize() override
    {
      GetOwner().lock()->AddComponent<Components::Transform>();
      GetOwner().lock()->AddComponent<Components::Collider>();
    }
    void PreUpdate(const float& dt) override {}

    void Update(const float& dt) override
    {
      if (m_pressed_) { m_color_ = Color(0.0f, 1.0f, 0.0f, 1.0f); }
      else { m_color_ = Color(1.0f, 0.0f, 0.0f, 1.0f); }
    }
    void PostUpdate(const float& dt) override {}
    void FixedUpdate(const float& dt) override {}
    void PreRender(const float& dt) override {}
    void Render(const float& dt) override {}
    void PostRender(const float& dt) override {}

    void Press() { m_pressed_ = !m_pressed_; }

  private:
    SERIALIZER_ACCESS
    ButtonScript()
      : m_pressed_(false) {}

    bool m_pressed_;
    Color m_color_;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::ButtonScript)