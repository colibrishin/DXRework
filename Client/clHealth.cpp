#include "pch.h"
#include "clHealth.h"

#include "clCharacterController.hpp"
#include "egCamera.h"
#include "egGlobal.h"
#include "egSceneManager.hpp"

namespace Client::Object
{
  HealthText::HealthText()
    : Text(GetResourceManager().GetResource<Resources::Font>("DefaultFont")) {}

  void HealthText::Initialize()
  {
    Text::Initialize();
    SetText("");
    SetPosition(Vector2(0.f, 64.f));
    SetColor(Vector4(1.f, 1.f, 1.f, 1.f));
    SetScale(1.f);
  }

  HealthText::~HealthText() {}

  void HealthText::PreUpdate(const float& dt) { Text::PreUpdate(dt); }

  void HealthText::Update(const float& dt)
  {
    Text::Update(dt);

    if (const auto parent = GetParent().lock())
    {
      if (const auto cc = parent->GetComponent<State::CharacterController>().lock())
      {
        SetText("Health: " + std::to_string(cc->GetHealth()));
      }
    }
  }

  void HealthText::PreRender(const float& dt) { Text::PreRender(dt); }

  void HealthText::Render(const float& dt)
  {
    if (const auto parent = GetParent().lock())
    {
      // naive local player check
      const auto scene = GetSceneManager().GetActiveScene().lock();
      const auto cam = scene->GetMainCamera().lock();
      const auto head = cam->GetParent().lock();

      // cc component -> head object -> player object
      if (head->GetParent().lock() != parent)
      {
        return;
      }
      else
      {
        if (const auto cc = parent->GetComponent<State::CharacterController>().lock())
        {
          Text::Render(dt);
        }
      }
    }
  }

  void HealthText::PostRender(const float& dt) { Text::PostRender(dt); }
}


