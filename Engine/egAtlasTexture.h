#pragma once
#include "egResource.h"
#include "egTexture2D.h"

namespace Engine::Resources
{
  class AtlasTexture : public Texture2D
  {
  public:
    RESOURCE_T(RES_T_ATLAS_TEX)

    AtlasTexture(const std::filesystem::path& path);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;

    eResourceType GetResourceType() const override;

    RESOURCE_SELF_INFER_GETTER(AtlasTexture)

    static inline boost::shared_ptr<AtlasTexture> Create(
      const std::string& name, const std::filesystem::path& path
    )
    {
      if (const auto ncheck = GetResourceManager().GetResource<AtlasTexture>(name).lock())
      {
        return ncheck;
      }

      if (const auto pcheck = GetResourceManager().GetResourceByRawPath<AtlasTexture>(path).lock())
      {
        return pcheck;
      }

      const auto obj = boost::make_shared<AtlasTexture>(path);
      GetResourceManager().AddResource(name, obj);
      return obj;
    }
     
  private:
    SERIALIZE_DECL
    AtlasTexture() : Texture2D("", {}) {}

  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AtlasTexture)