#pragma once
#include "egResource.h"
#include "egTexture3D.h"

namespace Engine::Resources
{
  class AnimationsTexture : public Texture3D
  {
  public:
    RESOURCE_T(RES_T_ANIMS_TEX)

    AnimationsTexture(const std::vector<StrongBoneAnimation>& animations);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;

    eResourceType GetResourceType() const override;

    RESOURCE_SELF_INFER_GETTER(AnimationsTexture)

    static inline boost::shared_ptr<AnimationsTexture> Create(
      const std::string& name, const std::vector<StrongBoneAnimation>& anims
    )
    {
      if (const auto ncheck = GetResourceManager().GetResource<AnimationsTexture>(name).lock()) { return ncheck; }

      const auto obj = boost::make_shared<AnimationsTexture>(anims);
      GetResourceManager().AddResource(name, obj);
      return obj;
    }

  protected:
    void loadDerived(ComPtr<ID3D12Resource>& res) override;

    bool map(char* mapped) override;
     
  private:
    SERIALIZE_DECL
    AnimationsTexture() : Texture3D("", {}) {}

    constexpr static size_t s_vec4_to_mat = 4;
    constexpr static size_t s_float_per_mat = sizeof(Matrix) / sizeof(float);

    GenericTextureDescription preEvaluateAnimations(const std::vector<StrongBoneAnimation> &anims, std::vector<std::vector<std::vector<Matrix>>> &preEvaluated);

    std::vector<StrongBoneAnimation> m_animations_;
    std::vector<std::vector<std::vector<Matrix>>> m_evaluated_animations_;

  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AnimationsTexture)