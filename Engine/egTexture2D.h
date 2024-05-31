#pragma once
#include "egTexture.h"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
  class Texture2D : public Texture
  {
  public:
    TEX_T(TEX_TYPE_2D)

    explicit Texture2D(const std::filesystem::path& path, const GenericTextureDescription& description)
      : Texture(path, TEX_TYPE_2D, description) { }
    ~Texture2D() override = default;

    RESOURCE_SELF_INFER_GETTER(Texture2D)

    static inline boost::shared_ptr<Texture2D> Create(
      const std::string&               name,
      const std::filesystem::path&     path,
      const GenericTextureDescription& desc
    )
    {
      if (const auto pcheck = GetResourceManager().GetResourceByRawPath<Texture2D>(path).lock(); 
          const auto ncheck = GetResourceManager().GetResource<Texture2D>(name).lock())
      {
        return ncheck;
      }
      const auto obj = boost::make_shared<Texture2D>(path, desc);
      GetResourceManager().AddResource(name, obj);
      return obj;
    }

    UINT GetWidth() const override;
    UINT GetHeight() const override;
    UINT GetDepth() const override;

  protected:
    void loadDerived(ComPtr<ID3D12Resource>& res) override;
    void Unload_INTERNAL() override;
    
  private:
    SERIALIZE_DECL
    Texture2D() : Texture("", TEX_TYPE_2D, {}) {}

    ComPtr<ID3D11Texture2D> m_tex_;
    
  };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture2D)