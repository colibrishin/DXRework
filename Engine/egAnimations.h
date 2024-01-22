#pragma once
#include "egResource.h"

namespace Engine::Resources
{
  class Animations : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_ANIMS)

    Animations(const std::vector<StrongBoneAnimation>& animations);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void OnDeserialized() override;

    eResourceType GetResourceType() const override;

    RESOURCE_SELF_INFER_GETTER(Animations)

    static inline boost::shared_ptr<Animations> Create(
      const std::string& name, const std::vector<StrongBoneAnimation>& anims
    )
    {
      if (const auto ncheck = GetResourceManager().GetResource<Animations>(name).lock()) { return ncheck; }

      const auto obj = boost::make_shared<Animations>(anims);
      GetResourceManager().AddResource(name, obj);
      return obj;
    }

  protected:
    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;

  private:
    std::vector<StrongBoneAnimation> m_animations_;

    UINT m_animation_count_;
    UINT m_max_bone_count_;
    UINT m_max_frame_count_;

    ComPtr<ID3D11Texture3D> m_tex_;
    ComPtr<ID3D11ShaderResourceView> m_srv_;
  };
}
