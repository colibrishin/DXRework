#pragma once
#include "egCommon.hpp"
#include "egManager.hpp"
#include "egResource.h"

namespace Engine::Manager
{
    class ResourceManager : public Abstract::Singleton<ResourceManager>
    {
    public:
        explicit ResourceManager(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void AddResource(const StrongResource& resource);
        void AddResource(const EntityName& name, const StrongResource& resource);

        template <typename T>
        boost::weak_ptr<T> GetResource(const EntityName& name)
        {
            if constexpr (std::is_base_of_v<Abstract::Resource, T>)
            {
                const auto target = m_resources_[typeid(T).name()];

                if (target.empty())
                {
                    return {};
                }

                const auto it =
                        std::ranges::find_if(
                                             target, [&name](const auto& resource)
                                             {
                                                 return resource->GetName() == name;
                                             });

                if (it != target.end())
                {
                    if (!(*it)->IsLoaded())
                    {
                        (*it)->Load();
                    }
                    return boost::dynamic_pointer_cast<T>(*it);
                }
            }

            return {};
        }

        WeakResource GetResource(const TypeName& type, const EntityName& name);

    private:
        std::map<const std::string, std::set<StrongResource>> m_resources_;
    };
} // namespace Engine::Manager
