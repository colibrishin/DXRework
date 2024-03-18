#include "pch.h"
#include "egAtlasTexture.h"

SERIALIZE_IMPL
(
 Engine::Resources::AtlasTexture,
 _ARTAG(_BSTSUPER(Engine::Resources::Texture2D))
)

namespace Engine::Resources
{
  AtlasTexture::AtlasTexture(const std::filesystem::path& path)
    : Texture2D(path, {}) {}

  void AtlasTexture::PreUpdate(const float& dt) {}

  void AtlasTexture::Update(const float& dt) {}

  void AtlasTexture::FixedUpdate(const float& dt) {}

  void AtlasTexture::PreRender(const float& dt) {}

  void AtlasTexture::Render(const float& dt)
  {
    BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_ATLAS, 0, SHADER_PIXEL);
    Texture2D::Render(dt);
  }

  void AtlasTexture::PostRender(const float& dt) { Texture2D::PostRender(dt); }

  void AtlasTexture::PostUpdate(const float& dt) {}

  void AtlasTexture::OnSerialized()
  {
    Texture2D::OnSerialized();
  }

  void AtlasTexture::OnDeserialized()
  {
    Texture2D::OnDeserialized();
  }

  eResourceType AtlasTexture::GetResourceType() const { return RES_T_ATLAS_TEX; }
}
