#pragma once
#include "egDebugger.hpp"

namespace Engine::Resources
{
  class Material final : public Abstract::Resource
  {
  public:
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

    void IgnoreAnimation(bool ignore) noexcept;

    template <
        typename T,
        typename U = boost::weak_ptr<T>,
        typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
    void SetResource(const std::string& name)
    {
      const auto search = GetResourceManager().GetResource<T>(name);

      if (search.expired()) { return; }

      static_assert(which_resource<T>::value != RES_T_MESH, 
                    "Mesh cannot be added as resource alone. Use with shape.");

      if constexpr (which_resource<T>::value == RES_T_SHADER)
      {
        m_shaders_.emplace_back(name);
        m_shaders_loaded_.emplace_back(search.lock());
        return;
      }

      m_resources_[which_resource<T>::value].push_back(name);
      m_resources_loaded_[which_resource<T>::value].push_back(search.lock());
    }

    void SetProperties(CBs::MaterialCB&& material_cb) noexcept;

    template <typename T, typename RLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
    [[nodiscard]] std::vector<boost::weak_ptr<T>> GetResources() const
    {
      if (!m_resources_loaded_.contains(which_resource<T>::value)) { return {}; }
      return m_resources_loaded_.at(which_resource<T>::value);
    }

    template <typename T, typename RLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
    [[nodiscard]] boost::weak_ptr<T> GetResource(const std::string& name = "") const
    {
      if (!m_resources_loaded_.contains(which_resource<T>::value)) { return {}; }
      if (m_resources_loaded_.at(which_resource<T>::value).empty()) { return {}; }
      if (name.empty()) { return boost::static_pointer_cast<T>(m_resources_loaded_.at(which_resource<T>::value).front()); }

      const auto it = std::ranges::find(m_resources_.at(which_resource<T>::value), name);

      if (it == m_resources_.at(which_resource<T>::value).end()) { return {}; }

      const auto idx = std::distance(m_resources_.at(which_resource<T>::value).begin(), it);
      return boost::static_pointer_cast<T>(m_resources_loaded_.at(which_resource<T>::value)[idx]);
    }

    void SetTextureSlot(const std::string& name, UINT slot);

    RESOURCE_SELF_INFER_GETTER(Material)
    RESOURCE_SELF_INFER_CREATE(Material)

  protected:
    SERIALIZER_ACCESS

    Material();
    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;

  private:
    CBs::MaterialCB m_material_cb_;
    bool            m_no_anim_flag_;

    std::vector<std::string>                                m_shaders_;
    std::map<const eResourceType, std::vector<std::string>> m_resources_;

    // non-serialized
    std::vector<StrongShader>                                  m_shaders_loaded_;
    std::map<const eResourceType, std::vector<StrongResource>> m_resources_loaded_;
  };
}
