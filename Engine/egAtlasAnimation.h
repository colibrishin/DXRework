#pragma once
#include "egBaseAnimation.h"
#include "egDXAnimCommon.hpp"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
  using namespace Engine::Graphics;

  class AtlasAnimation : public BaseAnimation
  {
  public:
    RESOURCE_T(RES_T_ATLAS_ANIM)

    AtlasAnimation(const AtlasAnimationPrimitive& primitive);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnDeserialized() override;
    void OnSerialized() override;
    void OnImGui() override;

    eResourceType GetResourceType() const override;

    void __vectorcall GetFrame(const float dt, AtlasAnimationPrimitive::AtlasFramePrimitive& out) const;

    RESOURCE_SELF_INFER_GETTER(AtlasAnimation)

    [[nodiscard]] static AtlasAnimationPrimitive ParseXML(const std::filesystem::path& path);

    static boost::shared_ptr<AtlasAnimation> Create(
      const std::string& name, const std::filesystem::path& xml_path
    )
    {
      if (const auto check = GetResourceManager().GetResource<AtlasAnimation>(name).lock())
      {
        return check;
      }

      // Parse the xml file
      const auto& primitive = ParseXML(xml_path);
      const auto obj = boost::make_shared<AtlasAnimation>(primitive);

      // Save primitive value for serialization and loading.
      obj->m_xml_path_ = xml_path.string();

      obj->SetTicksPerSecond(1.f);

      float duration = 0.f;
      primitive.GetTotalFrameDuration(duration);
      obj->SetDuration(duration);

      GetResourceManager().AddResource(name, obj);
      return obj;
    }

  protected:
    SERIALIZE_DECL

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;
    
    AtlasAnimation() = default;

    std::string m_xml_path_;
    AtlasAnimationPrimitive m_primitive_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AtlasAnimation)
