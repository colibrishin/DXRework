#pragma once
#include "egTexture.h"

namespace Engine::Resources
{
  class Texture3D : public Texture
  {
  public:
    TEX_T(TEX_TYPE_3D)

    explicit Texture3D(const std::filesystem::path& path, const GenericTextureDescription& description)
      : Texture(path, TEX_TYPE_3D, description) {}
    ~Texture3D() override = default;

    UINT GetWidth() const override final;
    UINT GetHeight() const override final;
    UINT GetDepth() const override final;

  protected:
    void loadDerived(ComPtr<ID3D11Resource>& res) override;
    void Unload_INTERNAL() override;
    
  private:
    SERIALIZE_DECL
    Texture3D() : Texture("", TEX_TYPE_3D, {}) {}

    UINT GetArraySize() const override final;

    ComPtr<ID3D11Texture3D> m_tex_;

  };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture3D)