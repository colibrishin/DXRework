#include "pch.h"
#include "egText.h"

#include "egImGuiHeler.hpp"
#include "egResourceManager.hpp"

SERIALIZE_IMPL
(
 Engine::Objects::Text,
 _ARTAG(_BSTSUPER(ObjectBase)) 
 _ARTAG(m_font_meta_path_str_) _ARTAG(m_position_)
 _ARTAG(m_color_) _ARTAG(m_scale_) _ARTAG(m_text_)
)

namespace Engine::Objects
{
  OBJ_CLONE_IMPL(Text)

  Text::Text(const WeakFont& font)
    : ObjectBase(DEF_OBJ_T_TEXT),
      m_rotation_radian_(0),
      m_scale_(1),
      m_font_(font.lock()) { SetCulled(false); }

  Text::~Text() = default;

  void Text::SetText(const std::string& text) { m_text_ = text; }

  void Text::SetPosition(const Vector2& position) { m_position_ = position; }

  void Text::SetColor(const Vector4& color) { m_color_ = color; }

  void Text::SetRotation(const float radian) { m_rotation_radian_ = radian; }

  void Text::SetScale(const float scale) { m_scale_ = scale; }

  void Text::OnSerialized()
  {
    ObjectBase::OnSerialized();

    Serializer::Serialize(m_font_->GetName(), m_font_);
    m_font_meta_path_str_ = m_font_->GetMetadataPath().string();
  }

  void Text::OnImGui()
  {
    ObjectBase::OnImGui();

    if (ImGui::Begin("Text Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      TextAligned("Text", m_text_);
      ImGuiVector2Editable("Position", GetID(), "m_position", m_position_);
      ImGuiColorEditable("Color", GetID(), "m_color_", m_color_);
      FloatAligned("Rotation", m_rotation_radian_);
      FloatAligned("Scale", m_scale_);

      TextDisabled("Font", m_font_->GetName());

      if (ImGui::BeginDragDropTarget())
      {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FONT"))
        {
          const StrongFont font = *static_cast<StrongFont*>(payload->Data);
          m_font_         = font;
        }
        ImGui::EndDragDropTarget();
      }

      ImGui::End();
    }
  }

  Text::Text()
    : ObjectBase(DEF_OBJ_T_TEXT),
      m_rotation_radian_(0),
      m_scale_(1) {}

  void Text::PreUpdate(const float& dt) { ObjectBase::PreUpdate(dt); }

  void Text::Update(const float& dt) { ObjectBase::Update(dt); }

  void Text::PreRender(const float& dt) { ObjectBase::PreRender(dt); }

  void Text::Render(const float& dt)
  {
    m_font_->SetText(m_text_);
    m_font_->SetPosition(m_position_);
    m_font_->SetColor(m_color_);
    m_font_->SetRotation(m_rotation_radian_);
    m_font_->SetScale(m_scale_);

    ObjectBase::Render(dt);
    m_font_->Render(dt);

    m_font_->SetText("");
    m_font_->SetPosition({0.0f, 0.0f});
    m_font_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_font_->SetRotation(0);
    m_font_->SetScale(1);
  }

  void Text::PostRender(const float& dt) { ObjectBase::PostRender(dt); }

  void Text::OnDeserialized()
  {
    ObjectBase::OnDeserialized();

    m_font_ = Resources::Font::GetByMetadataPath(m_font_meta_path_str_).lock();
  }
} // namespace Engine::Objects
