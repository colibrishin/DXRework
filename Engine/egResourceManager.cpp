#include "pch.h"
#include "egResourceManager.hpp"
#include <boost/mpl/for_each.hpp>
#include <boost/mp11/function.hpp>
#include <boost/type.hpp>

#include "egResourceManagerMeta.hpp"

namespace Engine::Manager
{
  void ResourceManager::Initialize() {}

  void ResourceManager::PreUpdate(const float& dt)
  {
    for (const auto& resources : m_resources_ | std::views::values)
    {
      for (const auto& res : resources) { if (res.use_count() == 1 && res->IsLoaded()) { res->Unload(); } }
    }
  }

  void ResourceManager::Update(const float& dt) {}

  void ResourceManager::PreRender(const float& dt) {}

  void ResourceManager::PostUpdate(const float& dt) {}

  void ResourceManager::Render(const float& dt) { this->OnImGui(); }

  void ResourceManager::PostRender(const float& dt) {}

  void ResourceManager::FixedUpdate(const float& dt) {}

  void ResourceManager::OnImGui()
  {
    if (ImGui::BeginMainMenuBar())
    {
      if (ImGui::BeginMenu("Load"))
      {
        // Second template parameter allows to use the type without instantiating it
        boost::mpl::for_each<LoadableResourceTypes, boost::type<boost::mpl::_>>(MetaLoadMenu());

        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    boost::mpl::for_each<LoadableResourceTypes, boost::type<boost::mpl::_>>(MetaResourceLoadDialog());

    if (ImGui::Begin("Resource Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      for (const auto& [type, resources] : m_resources_)
      {
        if (ImGui::TreeNode(g_resource_type_str[type]))
        {
          for (const auto& res : resources)
          {
            if (ImGui::Selectable(res->GetName().c_str()))
            {
              res->IsImGuiOpened() = !res->IsImGuiOpened();
            }

            if (ImGui::BeginDragDropSource())
            {
              ImGui::SetDragDropPayload("RESOURCE", &res, sizeof(res));
              ImGui::Text(res->GetName().c_str());
              ImGui::EndDragDropSource();
            }

            if (res->IsImGuiOpened())
            {
              const auto id = (res->GetName() + "###" + std::to_string(res->GetID()));

              if (ImGui::Begin(id.c_str(), &res->IsImGuiOpened(), ImGuiWindowFlags_AlwaysAutoResize))
              {
                res->OnImGui();
                ImGui::End();
              }
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
