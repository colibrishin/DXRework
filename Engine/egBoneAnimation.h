#pragma once
#include "egBaseAnimation.h"
#include "egDXAnimCommon.hpp"
#include "egStructuredBuffer.hpp"

namespace Engine::Resources
{
  using namespace Engine::Graphics;

  class BoneAnimation : public BaseAnimation
  {
  public:
    RESOURCE_T(RES_T_BONE_ANIM)

    BoneAnimation(const AnimationPrimitive& primitive);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void OnDeserialized() override;

    void          BindBone(const WeakBone& bone_info);
    eResourceType GetResourceType() const override;

    std::vector<Matrix> GetFrameAnimation(float dt);

    RESOURCE_SELF_INFER_GETTER(BoneAnimation)

  protected:
    SERIALIZER_ACCESS

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;

  private:
    BoneAnimation();

    AnimationPrimitive m_primitive_;
    StrongBone         m_bone_;

    // non-serialized
    float               m_evaluated_time_;
    std::vector<Matrix> m_evaluated_data_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BoneAnimation)
