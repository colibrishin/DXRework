#pragma once
#include "egDebugger.hpp"

namespace Engine::Resources
{
    class Material final : public Abstract::Resource
    {
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_MTR);

        Material();

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
        void Set(const std::string& name)
        {
            const auto search = GetResourceManager().GetResource<T>(name);

            if (search.expired()) return;

            m_resources_[which_resource<T>::value] = name;
            m_resources_loaded_[which_resource<T>::value] = search.lock();
        }

    protected:
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;

    private:
        std::map<const eResourceType, std::string> m_resources_;

        // non-serialized
        std::map<const eResourceType, StrongResource> m_resources_loaded_;

    };
}
