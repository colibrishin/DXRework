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
    void OnDeserialized() override;

    void SetTempParam(TempParam&& param) noexcept;
    bool IsRenderDomain(const eShaderDomain domain) const noexcept;

    template <typename T, typename U = boost::weak_ptr<T>, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
    void SetResource(const std::string& name)
    {
      const auto search = GetResourceManager().GetResource<T>(name);

      if (search.expired()) { return; }

      static_assert
        (
         which_resource<T>::value != RES_T_MESH,
         "Mesh sole cannot added into material, wrap it with shape instead."
        );

      if constexpr (which_resource<T>::value == RES_T_SHADER)
      {
        m_shaders_.emplace_back(name);
        m_shaders_loaded_[search.lock()->template GetSharedPtr<Shader>()->GetDomain()] = search.lock();
        return;
      }

      m_resources_[which_resource<T>::value].push_back(name);
      m_resources_loaded_[which_resource<T>::value].push_back(search.lock());
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

      const auto it = std::ranges::find(m_resources_.at(which_resource<T>::value), name);

      if (it == m_resources_.at(which_resource<T>::value).end()) { return {}; }

      const auto idx = std::distance(m_resources_.at(which_resource<T>::value).begin(), it);
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

  private:
    SERIALIZER_ACCESS
    Material();

    CBs::MaterialCB m_material_cb_;

    std::vector<std::string>                                m_shaders_;
    std::map<const eResourceType, std::vector<std::string>> m_resources_;

    // non-serialized
    TempParam                                                  m_temp_param_;
    std::map<const eShaderDomain, StrongShader>                m_shaders_loaded_;
    std::map<const eResourceType, std::vector<StrongResource>> m_resources_loaded_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Material)