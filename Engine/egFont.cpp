#include "pch.h"
#include "egFont.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include "egSerialization.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::Font,
 _ARTAG(_BSTSUPER(Resource)) _ARTAG(m_position_)
 _ARTAG(m_color_) _ARTAG(m_rotation_radian_)
 _ARTAG(m_scale_) _ARTAG(m_text_)
)

namespace Engine::Resources
{
  Font::Font(const std::filesystem::path& path)
    : Resource(path, RES_T_FONT),
      m_rotation_radian_(0),
      m_scale_(1),
      m_lazy_reload_(false) {}

  void Font::Initialize() {}

  void Font::PreUpdate(const float& dt) {}

  void Font::Update(const float& dt) {}

  void Font::PreRender(const float& dt) { }

  void Font::PostUpdate(const float& dt)
  {
    if (m_lazy_reload_)
    {
      Unload();
      Load();
      m_lazy_reload_ = false;
    }
  }

  void Font::Render(const float& dt)
  {
    m_font_->DrawString
      (
       GetToolkitAPI().GetSpriteBatch(), m_text_.c_str(),
       m_position_, m_color_, m_rotation_radian_, Vector2::Zero,
       m_scale_
      );
  }

  void Font::FixedUpdate(const float& dt) {}

  void Font::PostRender(const float& dt) {}

  void Font::SetText(const std::string& text) { m_text_ = text; }

  void Font::SetPosition(const Vector2& position) { m_position_ = position; }

  void Font::SetColor(const Vector4& color) { m_color_ = color; }

  void Font::SetRotation(const float radian) { m_rotation_radian_ = radian; }

  void Font::SetScale(const float scale) { m_scale_ = scale; }

  void Font::ChangeFont(const std::filesystem::path& path)
  {
    SetPath(path);
    m_lazy_reload_ = true;
  }

  void Font::OnSerialized() {}

  void Font::OnDeserialized() { Resource::OnDeserialized(); }

  void Font::Unload_INTERNAL() { m_font_.reset(); }

  Font::Font()
    : Resource("", RES_T_FONT),
      m_rotation_radian_(0),
      m_scale_(1),
      m_lazy_reload_(false) {}

  void Font::Load_INTERNAL()
  {
    m_font_ = std::make_unique<SpriteFont>
      (
       GetD3Device().GetDevice(),
       GetPath().c_str()
      );
  }
} // namespace Engine::Resources
