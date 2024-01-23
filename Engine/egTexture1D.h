#pragma once
#include "egTexture.h"

namespace Engine::Resources
{
  class Texture1D : Texture
  {
  public:
    TEX_T(TEX_TYPE_1D)

    explicit Texture1D(const std::filesystem::path& path, const GenericTextureDescription& description)
      : Texture(path, TEX_TYPE_3D, description) { }
    ~Texture1D() override = default;

    void          OnDeserialized() override;
    void          OnImGui() override;

    UINT GetWidth() const override final;

  protected:
    void loadDerived(ComPtr<ID3D11Resource>& res) override;
    void Unload_INTERNAL() override;

  private:
    UINT GetHeight() const override final;
    UINT GetDepth() const override final;
    UINT GetArraySize() const override final;

    ComPtr<ID3D11Texture1D> m_tex_;

  };
}
