#pragma once
#include "egDebugger.hpp"

namespace Engine::Resources
{
    class Material final : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_MTR);

        Material(const std::filesystem::path& path);
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        template <typename T, typename U = boost::weak_ptr<T>, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
        void SetResource(const std::string& name)
        {
            const auto search = GetResourceManager().GetResource<T>(name);

            if (search.expired()) return;

            static_assert(which_resource<T>::value != RES_T_MESH || which_resource<T>::value != RES_T_SHAPE,
                          "Define shape in the ModelRenderer");

            if constexpr (which_resource<T>::value == RES_T_SHADER)
            {
                m_shaders_[convert_shaderT_enum<T>::value_e()] = name;
                m_shaders_loaded_[convert_shaderT_enum<T>::value_e()] = search.lock();
                return;
            }

            m_resources_[which_resource<T>::value].push_back(name);
            m_resources_loaded_[which_resource<T>::value].push_back(search.lock());
        }

        void SetProperties(CBs::MaterialCB&& material_cb) noexcept;

        template <typename T>
        [[nodiscard]] auto GetResources() const
        {
            if (!m_resources_loaded_.contains(which_resource<T>::value)) 
                return std::vector<StrongResource>{};
            return m_resources_loaded_.at(which_resource<T>::value);
        }

        template <typename T>
        [[nodiscard]] boost::weak_ptr<T> GetResource(const std::string& name) const
        {
            if (!m_resources_loaded_.contains(which_resource<T>::value)) return {};
            if (m_resources_loaded_.at(which_resource<T>::value).empty()) return {};

            const auto it = std::ranges::find(m_resources_.at(which_resource<T>::value), name);

            if (it == m_resources_.at(which_resource<T>::value).end()) return {};

            const auto idx = std::distance(m_resources_.at(which_resource<T>::value).begin(), it);
            return boost::reinterpret_pointer_cast<T>(m_resources_loaded_.at(which_resource<T>::value)[idx]);
        }

        void SetTextureSlot(const std::string& name, const UINT slot);

        RESOURCE_SELF_INFER_GETTER(Material)
        RESOURCE_SELF_INFER_CREATE(Material)

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        bool ShaderCheck() const;

        CBs::MaterialCB m_material_cb_;
        std::map<const eShaderType, std::string> m_shaders_;
        std::map<const eResourceType, std::vector<std::string>> m_resources_;

        // non-serialized
        std::map<const eShaderType, StrongShader> m_shaders_loaded_;
        std::map<const eResourceType, std::vector<StrongResource>> m_resources_loaded_;

    };
}
