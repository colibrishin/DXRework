#include "pch.h"
#include "egResourceManager.h"

namespace Engine::Manager
{
    void ResourceManager::Initialize() {}

    void ResourceManager::PreUpdate(const float& dt)
    {
        for (const auto& resources : m_resources_ | std::views::values)
        {
            for (const auto& res : resources)
            {
                if (res.use_count() == 1 && res->IsLoaded())
                {
                    res->Unload();
                }
            }
        }
    }

    void ResourceManager::Update(const float& dt) {}

    void ResourceManager::PreRender(const float& dt) {}

    void ResourceManager::Render(const float& dt) {}

    void ResourceManager::PostRender(const float& dt) {}

    void ResourceManager::FixedUpdate(const float& dt) {}

    void ResourceManager::AddResource(const StrongResource& resource)
    {
        m_resources_[resource->GetTypeName()].insert(resource);
    }

    void ResourceManager::AddResource(const EntityName& name, const StrongResource& resource)
    {
        m_resources_[resource->GetTypeName()].insert(resource);
        resource->SetName(name);
    }

    WeakResource ResourceManager::GetResource(const TypeName& type, const EntityName& name)
    {
        const auto target = m_resources_[type];

        if (target.empty())
        {
            return {};
        }

        const auto                                                       it = std::ranges::find_if(
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
            return *it;
        }

        return {};
    }
}
