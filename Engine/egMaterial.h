#pragma once
#include "egDXCommon.h"
#include "egDebugger.hpp"
#include "egResourceManager.hpp"

namespace Engine::Resources
{
  class Material final : public Abstract::Resource
  {
  public:
    struct TempParam
    {
      UINT  instanceCount = 1;
      bool  bypassShader = false;
      eShaderDomain domain = SHADER_DOMAIN_OPAQUE;
    };

    RESOURCE_T(RES_T_MTR);

    Material(const std::filesystem::path& path);

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    void SetTempParam(TempParam&& param) noexcept;
    bool IsRenderDomain(const eShaderDomain domain) const noexcept;

    template <typename T, typename U = boost::weak_ptr<T>, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
    void SetResource(const std::string& name)
    {
      const auto search = GetResourceManager().GetResource<T>(name);

      if (search.expired()) { return; }

      SetResource(search.lock());
    }

    void SetProperties(CBs::MaterialCB&& material_cb) noexcept;

    template <typename T>
    [[nodiscard]] auto GetResources() const
    {
      if (!m_resources_loaded_.contains(which_resource<T>::value)) { return std::vector<StrongResource>{}; }
      return m_resources_loaded_.at(which_resource<T>::value);
    }

    template <typename T>
    [[nodiscard]] boost::weak_ptr<T> GetResource(const std::string& name) const
    {
      if (!m_resources_loaded_.contains(which_resource<T>::value)) { return {}; }
      if (m_resources_loaded_.at(which_resource<T>::value).empty()) { return {}; }

      const auto it = std::ranges::find(m_resources_loaded_.at(which_resource<T>::value), name);

      if (it == m_resources_loaded_.at(which_resource<T>::value).end()) { return {}; }

      const auto idx = std::distance(m_resources_loaded_.at(which_resource<T>::value).begin(), it);
      return boost::reinterpret_pointer_cast<T>(m_resources_loaded_.at(which_resource<T>::value)[idx]);
    }

    template <typename T>
    [[nodiscard]] boost::weak_ptr<T> GetResource(UINT idx) const
    {
      if (!m_resources_loaded_.contains(which_resource<T>::value)) { return {}; }
      if (m_resources_loaded_.at(which_resource<T>::value).empty()) { return {}; }

      const auto& anims = m_resources_loaded_.at(which_resource<T>::value);

      return boost::static_pointer_cast<T>(*(anims.begin() + idx));
    }

    void SetTextureSlot(const std::string& name, UINT slot);

    RESOURCE_SELF_INFER_GETTER(Material)
    RESOURCE_SELF_INFER_CREATE(Material)

  protected:
    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;
    RESOURCE_SERIALIZER_OVERRIDE(Material)

  private:
    SERIALIZER_ACCESS
    Material();

    void SetResource(const StrongResource& resource);

    CBs::MaterialCB m_material_cb_;

    std::vector<std::pair<EntityName, MetadataPathStr>>                                m_shader_paths_;
    std::map<const eResourceType, std::vector<std::pair<EntityName, MetadataPathStr>>> m_resource_paths_;

    // non-serialized
    bool                                                       m_b_edit_dialog_;
    TempParam                                                  m_temp_param_;
    std::map<const eShaderDomain, StrongShader>                m_shaders_loaded_;
    std::map<const eResourceType, std::vector<StrongResource>> m_resources_loaded_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Material)