#pragma once
#include "egCommon.hpp"
#include "egManager.hpp"
#include "egResource.h"

namespace Engine::Manager
{
    class ResourceManager : public Abstract::Singleton<ResourceManager>
    {
    public:
        explicit ResourceManager(SINGLETON_LOCK_TOKEN) {}

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void OnImGui() override;

        template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
        void AddResource(const boost::shared_ptr<T>& resource)
        {
            m_resources_[which_resource<T>::value].insert(resource);
        }

        template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
        void AddResource(const EntityName& name, const boost::shared_ptr<T>& resource)
        {
            m_resources_[which_resource<T>::value].insert(resource);
            resource->SetName(name);
        }

        template <typename T>
        boost::weak_ptr<T> GetResource(const EntityName& name)
        {
            auto& resources = m_resources_[which_resource<T>::value];
            auto it = std::find_if(resources.begin(), resources.end(), [&name](const StrongResource& resource) {
                return resource->GetName() == name;
            });

            if (it != resources.end())
            {
                if (!(*it)->IsLoaded())
                {
                    (*it)->Load();
                }

                return boost::reinterpret_pointer_cast<T>(*it);
            }

            return {};
        }

        WeakResource GetResource(const EntityName& name, const eResourceType& type)
        {
            auto&      resources = m_resources_[type];
            const auto it        = std::ranges::find_if(
                                                 resources
                                                 , [&name](const StrongResource& resource)
                                                 {
                                                     return resource->GetName() == name;
                                                 });

            if (it != resources.end())
            {
                if (!(*it)->IsLoaded())
                {
                    (*it)->Load();
                }

                return *it;
            }

            return {};
        }

        template <typename T>
        boost::weak_ptr<T> GetResourceByPath(const std::filesystem::path& path)
        {
            auto& resources = m_resources_[which_resource<T>::value];
            auto it = std::find_if(
                                   resources.begin(), resources.end(), [&path](const StrongResource& resource)
                                   {
                                       return resource->GetPath() == path;
                                   });

            if (it != resources.end())
            {
                if (!(*it)->IsLoaded())
                {
                    (*it)->Load();
                }

                return boost::reinterpret_pointer_cast<T>(*it);
            }

            return {};
        }

    private:
        friend struct SingletonDeleter;
        ~ResourceManager() override = default;

        std::map<eResourceType, std::set<StrongResource>> m_resources_;
    };
} // namespace Engine::Manager
