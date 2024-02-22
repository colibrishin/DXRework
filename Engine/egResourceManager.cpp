#include "pch.h"
#include "egResourceManager.hpp"
#include <boost/mpl/for_each.hpp>
#include <boost/mp11/function.hpp>
#include <boost/type.hpp>

#include "egImGuiHeler.hpp"
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
      if (ImGui::BeginMenu("New"))
      {
        if (ImGui::MenuItem("Texture")) 
        {
          m_b_imgui_load_texture_dialog_ = true;
        }

        if (ImGui::MenuItem("Shape")) 
        {
          m_b_imgui_load_shape_dialog_ = true;
        }

        if (ImGui::MenuItem("Sound"))
        {
          m_b_imgui_load_sound_dialog_ = true;
        }

        if (ImGui::MenuItem("Shader"))
        {
          m_b_imgui_load_shader_dialog_ = true;
        }

        if (ImGui::MenuItem("Font"))
        {
          m_b_imgui_load_font_dialog_ = true;
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Load"))
      {
        // Second template parameter allows to use the type without instantiating it
        boost::mpl::for_each<LoadableResourceTypes, boost::type<boost::mpl::_>>(MetaLoadMenu());

        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    OpenNewTextureDialog();
    OpenNewSimpleDialog<Resources::Shape>(m_b_imgui_load_shape_dialog_);
    OpenNewSimpleDialog<Resources::Sound>(m_b_imgui_load_sound_dialog_);
    OpenNewSimpleDialog<Resources::Font>(m_b_imgui_load_font_dialog_);

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

  void ResourceManager::OpenNewTextureDialog()
  {
    if (m_b_imgui_load_texture_dialog_)
    {
      if (ImGui::Begin("New Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        static char name_buffer[256] = {};
        static char path_buffer[256] = {};
        static bool load_from_file = true;

        CheckboxAligned("From File", load_from_file);
        if (load_from_file)
        {
          ImGui::InputText("Name", name_buffer, 256);
          ImGui::InputText("Path", path_buffer, 256);

          if (ImGui::Button("Load"))
          {
            try
            {
              Resources::Texture2D::Create(name_buffer, path_buffer, {});
              m_b_imgui_load_texture_dialog_ = false;
            }
            catch (const std::exception& e)
            {
              ImGui::SameLine();
              ImGui::Text(e.what());
            }
          }

          if (ImGui::Button("Cancel")) { m_b_imgui_load_texture_dialog_ = false; }
        }
        else
        {
          static Resources::Texture::GenericTextureDescription desc
          {
            .Width = 0,
            .Height = 0,
            .Depth = 0,
            .ArraySize = 0,
            .Format = DXGI_FORMAT_UNKNOWN,
            .CPUAccessFlags = 0,
            .BindFlags = 0,
            .MipsLevel = 0,
            .MiscFlags = 0,
            .Usage = D3D11_USAGE_DEFAULT,
            .SampleDesc = {.Count = 1, .Quality = 0}
          };

          UINTAligned("Width", desc.Width);
          UINTAligned("Height", desc.Height);
          UINTAligned("Depth", desc.Depth);
          UINTAligned("Array Size", desc.ArraySize);
          // list
          // list
          // list
          UINTAligned("Mips Level", desc.MipsLevel);
          // list
          // list
          UINTAligned("Sampler Count", desc.SampleDesc.Count);
          UINTAligned("Sampler Quality", desc.SampleDesc.Quality);
        }

        if (ImGui::Button("Load"))
        {
          using TextureList = boost::mpl::vector<Resources::Texture1D, Resources::Texture3D, Resources::Texture3D>;

          // test texture type (by description)
          // Create texture and try catch
        }

        if (ImGui::Button("Cancel")) { m_b_imgui_load_texture_dialog_ = false; }

        ImGui::End();
      }
    }
  }

  void ResourceManager::OpenNewShaderDialog()
  {
    if (m_b_imgui_load_shader_dialog_)
    {
      
    }
  }
}
