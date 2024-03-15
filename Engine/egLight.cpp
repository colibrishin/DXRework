#include "pch.h"
#include "egLight.h"
#include "egCamera.h"
#include "egManagerHelper.hpp"

SERIALIZE_IMPL
(
 Engine::Objects::Light,
 _ARTAG(_BSTSUPER(ObjectBase)) _ARTAG(m_color_)
 _ARTAG(m_type_)
)

namespace Engine::Objects
{
  OBJ_CLONE_IMPL(Light)

  Light::Light()
    : ObjectBase(DEF_OBJ_T_LIGHT),
      m_range_(10.f),
      m_type_(LIGHT_T_DIRECTIONAL) {}

  Light::~Light() {}

  void Light::SetColor(Vector4 color) { m_color_ = color; }

  void Light::SetType(eLightType type)
  {
    m_type_ = type;

    if (m_type_ == LIGHT_T_DIRECTIONAL)
      SetRange(0.0f);
    else
      SetRange(10.0f);      
  }

  void Light::SetRange(float range)
  {
    if (m_type_ == LIGHT_T_DIRECTIONAL)
      return;

    m_range_ = range;
  }

  void Light::Initialize()
  {
    AddComponent<Components::Transform>();
    m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
    SetCulled(false);
  }

  void Light::PreUpdate(const float& dt) { ObjectBase::PreUpdate(dt); }

  void Light::Update(const float& dt) { ObjectBase::Update(dt); }

  void Light::PreRender(const float& dt) { ObjectBase::PreRender(dt); }

  void Light::Render(const float& dt)
  {
    ObjectBase::Render(dt);
#ifdef _DEBUG
    const auto tr = GetComponent<Components::Transform>().lock();

    const BoundingSphere sphere(tr->GetWorldPosition(), 0.5f);
    GetDebugger().Draw(sphere, DirectX::Colors::Yellow);
#endif
  }

  void Light::PostRender(const float& dt) { ObjectBase::PostRender(dt); }

  void Light::PostUpdate(const float& dt) { ObjectBase::PostUpdate(dt); }

  void Light::OnDeserialized() { ObjectBase::OnDeserialized(); }

  void Light::OnImGui()
  {
    ObjectBase::OnImGui();

    const auto title = GetName() + "###" + std::to_string(GetID());

    if (ImGui::Begin(title.c_str(), &IsImGuiOpened()))
    {
      ImGui::Text("Light Type");
      ImGui::SameLine();
      ImGui::PushItemWidth(100);
      ImGui::Combo("##LightType", reinterpret_cast<int*>(&m_type_), "Unknown\0Directional\0Spot\0");
      ImGui::PopItemWidth();

      ImGui::Text("Color");
      ImGui::SameLine();
      ImGui::ColorEdit4("##Color", &m_color_.x);

      if (m_type_ != LIGHT_T_DIRECTIONAL)
      {
        ImGui::Text("Range");
        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        ImGui::DragFloat("##Range", &m_range_, 0.1f, 0.0f, 100.0f);
        ImGui::PopItemWidth();
      }

      ImGui::End();
    }
  }
} // namespace Engine::Objects
