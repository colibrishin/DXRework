#include "pch.h"
#include "egFont.h"

#include "egSerialization.hpp"

SERIALIZE_IMPL
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
      m_lazy_reload_(false),
      m_heap_(GetD3Device().GetDevice(), 1) {}

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
    GetToolkitAPI().AppendSpriteBatch
      (
       [this]()
       {
         m_font_->DrawString
           (
            GetToolkitAPI().GetSpriteBatch(), m_text_.c_str(),
            m_position_, m_color_, m_rotation_radian_, Vector2::Zero,
            m_scale_
           );
       }
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
      m_lazy_reload_(false),
      m_heap_(GetD3Device().GetDevice(), 1) {}

  void Font::Load_INTERNAL()
  {
    DirectX::ResourceUploadBatch resource_upload_batch(GetD3Device().GetDevice());

    resource_upload_batch.Begin();

    m_font_ = std::make_unique<SpriteFont>
      (
       GetD3Device().GetDevice(),
       resource_upload_batch,
       GetPath().c_str(),
       m_heap_.GetFirstCpuHandle(),
       m_heap_.GetFirstGpuHandle()
      );

    const auto& token = resource_upload_batch.End(GetD3Device().GetCopyCommandQueue());

    token.wait();
  }
} // namespace Engine::Resources
