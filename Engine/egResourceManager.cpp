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

    OpenNewSimpleDialog<Resources::Texture2D>(m_b_imgui_load_texture_dialog_, Resources::Texture::GenericTextureDescription{});
    OpenNewSimpleDialog<Resources::Shape>(m_b_imgui_load_shape_dialog_);
    OpenNewSimpleDialog<Resources::Sound>(m_b_imgui_load_sound_dialog_);
    OpenNewSimpleDialog<Resources::Font>(m_b_imgui_load_font_dialog_);
    OpenNewShaderDialog();

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

  void ResourceManager::OpenNewShaderDialog()
  {
    if (m_b_imgui_load_shader_dialog_)
    {
      if (ImGui::Begin("New Shader", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        static char name_buffer[256] = {};
        static char path_buffer[256] = {};

        static const char* domain_chars[] = {"Opaque", "Mask", "Transparent", "Postprocessing"};
        static const char* depth_flag[]   = {"None", "All"};
        static const char* depth_func[]   = {
          "Never", "Less", "Equal", "LessEqual", "Greater", "NotEqual", "GreaterEqual", "Always"
        };
        static const char* cull_flag[]   = {"None", "Front", "Back"};
        static const char* fill_flag[]   = {"Wireframe", "Solid"};
        static const char* filter_func[] = {
          "MinMagMipPoint", "MinMagPointMipLinear", "MinPointMagLinearMipPoint", "MinPointMagMipLinear",
          "MinLinearMagMipPoint", "MinLinearMagPointMipLinear", "MinMagLinearMipPoint", "MinMagMipLinear", "Anisotropic"
        };
        static const char* sampler_address[] = {"Wrap", "Mirror", "Clamp", "Border", "MirrorOnce"};
        static const char* sampler_func[]    = {
          "Never", "Less", "Equal", "LessEqual", "Greater", "NotEqual", "GreaterEqual", "Always"
        };

        int sel_domain          = 0;
        int sel_depth           = 0;
        int sel_depth_func      = 0;
        int sel_cull            = 0;
        int sel_fill            = 0;
        int sel_filter          = 0;
        int sel_sampler_address = 0;
        int sel_sampler_func    = 0;

        ImGui::InputText("Name", name_buffer, 256);
        ImGui::InputText("Path", path_buffer, 256);

        ImGui::Combo("Domain", &sel_domain, domain_chars, IM_ARRAYSIZE(domain_chars));
        ImGui::Combo("Depth", &sel_depth, depth_flag, IM_ARRAYSIZE(depth_flag));
        ImGui::Combo("Depth Func", &sel_depth_func, depth_func, IM_ARRAYSIZE(depth_func));
        ImGui::Combo("Cull", &sel_cull, cull_flag, IM_ARRAYSIZE(cull_flag));
        ImGui::Combo("Fill", &sel_fill, fill_flag, IM_ARRAYSIZE(fill_flag));
        ImGui::Combo("Filter", &sel_filter, filter_func, IM_ARRAYSIZE(filter_func));
        ImGui::Combo("Sampler Address", &sel_sampler_address, sampler_address, IM_ARRAYSIZE(sampler_address));
        ImGui::Combo("Sampler Func", &sel_sampler_func, sampler_func, IM_ARRAYSIZE(sampler_func));

        if (ImGui::Button("Load"))
        {
          try
          {
            Resources::Shader::Create
              (
               name_buffer,
               path_buffer,
               (eShaderDomain)(sel_domain),
               (UINT)(sel_depth | sel_depth_func >> 2),
               (UINT)(sel_cull | sel_fill >> 3),
               (D3D11_FILTER)(sel_filter),
               (UINT)(sel_sampler_address | sel_sampler_func >> 5)
              );

            m_b_imgui_load_shader_dialog_ = false;
            std::memset(name_buffer, 0, 256);
            std::memset(path_buffer, 0, 256);
          }
          catch (const std::exception& e)
          {
            ImGui::SameLine();
            ImGui::Text(e.what());
            std::memset(name_buffer, 0, 256);
            std::memset(path_buffer, 0, 256);
          }
        }

        if (ImGui::Button("Cancel"))
        {
          m_b_imgui_load_shader_dialog_ = false;
          std::memset(name_buffer, 0, 256);
          std::memset(path_buffer, 0, 256);;
        }

        ImGui::End();
      }
    }
  }
}
