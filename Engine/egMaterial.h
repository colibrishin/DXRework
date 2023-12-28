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
            }

            m_resources_[which_resource<T>::value] = name;
            m_resources_loaded_[which_resource<T>::value] = search.lock();
        }

        void SetProperties(CBs::MaterialCB&& material_cb) noexcept;

        RESOURCE_SELF_INFER_GETTER(Material)
        RESOURCE_SELF_INFER_CREATE(Material)

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        bool ShaderCheck() const;

        CBs::MaterialCB m_material_cb_;
        std::map<const eShaderType, std::string> m_shaders_;
        std::map<const eResourceType, std::string> m_resources_;

        // non-serialized
        std::map<const eShaderType, StrongShader> m_shaders_loaded_;
        std::map<const eResourceType, StrongResource> m_resources_loaded_;

    };
}
