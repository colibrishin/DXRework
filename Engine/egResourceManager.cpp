#include "pch.h"
#include "egResourceManager.hpp"

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

    void ResourceManager::PostUpdate(const float& dt) {}

    void ResourceManager::Render(const float& dt)
    {
        this->OnImGui();
    }

    void ResourceManager::PostRender(const float& dt) {}

    void ResourceManager::FixedUpdate(const float& dt) {}

    void ResourceManager::OnImGui()
    {
        if (ImGui::Begin("Resource Manager"))
        {
            for (const auto& resources : m_resources_)
            {
                if (ImGui::TreeNode(std::to_string(resources.first).c_str()))
                {
                    for (const auto& res : resources.second)
                    {
                                               if (ImGui::TreeNode(res->GetName().c_str()))
                                               {
                                                                              res->OnImGui();
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                    ImGui::Separator();
                }
            }

            ImGui::End();
        }
    }
}
