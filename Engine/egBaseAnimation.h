#pragma once
#include "egDXAnimCommon.hpp"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
  using namespace Engine::Graphics;

  class BaseAnimation : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_BASE_ANIM)

    BaseAnimation(const BoneAnimationPrimitive& primitive);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnDeserialized() override;
    void OnSerialized() override;

    void  SetTicksPerSecond(const float& ticks_per_second);
    void  SetDuration(const float& duration);
    float GetTicksPerSecond() const;
    float GetDuration() const;

    static float ConvertDtToFrame(const float & dt, const float ticks_per_second);

    RESOURCE_SELF_INFER_GETTER(BaseAnimation)

    static boost::shared_ptr<BaseAnimation> Create(
      const std::string& name, const BoneAnimationPrimitive& primitive
    )
    {
      if (const auto check = GetResourceManager().GetResource<BaseAnimation>(name).lock()) { return check; }
      const auto obj = boost::make_shared<BaseAnimation>(primitive);
      GetResourceManager().AddResource(name, obj);
      return obj;
    }

  protected:
    friend class Components::Animator;

    SERIALIZE_DECL

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;
    
    BaseAnimation();

    float m_ticks_per_second_;
    float m_duration_;

    BoneAnimationPrimitive m_simple_primitive_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::BaseAnimation)
