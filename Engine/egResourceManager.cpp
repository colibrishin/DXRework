#include "pch.h"
#include "egResourceManager.hpp"
#include <boost/mpl/for_each.hpp>
#include <boost/mp11/function.hpp>
#include <boost/type.hpp>

#include "egImGuiHeler.hpp"
#include "egResourceManagerMeta.hpp"

#include "egResource.h"

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

        if (ImGui::MenuItem("Atlas"))
        {
          m_b_imgui_load_atlas_dialog_ = true;
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
    OpenNewAtlasDialog();

    boost::mpl::for_each<LoadableResourceTypes, boost::type<boost::mpl::_>>(MetaResourceLoadDialog());

    if (ImGui::Begin("Resource Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      for (const auto& [type, resources] : m_resources_)
      {
        if (ImGui::TreeNode(g_resource_type_str[type]))
        {
          for (const auto& res : resources)
          {
            const auto id = (res->GetName() + "###" + std::to_string(res->GetID()));

            if (ImGui::Selectable(id.c_str()))
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

  WeakResource ResourceManager::GetResourceByRawPath(const std::filesystem::path& path, const eResourceType type)
  {
    if (path.empty()) { return {}; }

    auto& resources = m_resources_[type];
    auto  it        = std::find_if
      (
       resources.begin(), resources.end(), [&path](const StrongResource& resource)
       {
         return resource->GetPath() == path;
       }
      );

    if (it != resources.end())
    {
      if (!(*it)->IsLoaded()) { (*it)->Load(); }

      return *it;
    }

    return {};
  }

  WeakResource ResourceManager::GetResourceByMetadataPath(
    const std::filesystem::path& path, const eResourceType type
  )
  {
    if (path.empty())
    {
      return {};
    }

    auto& resources = m_resources_[type];
    auto  it        = std::find_if
      (
       resources.begin(), resources.end(), [&path](const StrongResource& resource)
       {
         return resource->GetMetadataPath() == path;
       }
      );

    if (it != resources.end())
    {
      if (!(*it)->IsLoaded()) { (*it)->Load(); }

      return *it;
    }
    else
    {
      if (std::filesystem::exists(path))
      {
        const auto res = Serializer::Deserialize<Entity>(path.generic_string())->GetSharedPtr<Abstract::Resource>();
        m_resources_[type].insert(res);
        res->Load();
        return res;
      }
    }

    return {};
  }

  bool ResourceManager::RequestMultipleChoiceDialog()
  {
    if (m_b_imgui_multiple_choice_dialog_)
    {
      return false;
    }

    m_b_imgui_multiple_choice_dialog_ = true;
    return true;
  }

  bool ResourceManager::OpenMultipleChoiceDialog(std::vector<StrongResource>& selected)
  {
    if (m_b_imgui_multiple_choice_dialog_)
    {
      if (ImGui::Begin("Multiple Choice", &m_b_imgui_multiple_choice_dialog_, ImGuiWindowFlags_AlwaysAutoResize))
      {
        for (const auto& [type, resources] : m_resources_)
        {
          if (ImGui::TreeNode(g_resource_type_str[type]))
          {
            for (const auto& res : resources)
            {
              const auto it     = std::ranges::find(selected, res);
              const bool chosen = it != selected.end();

              if (ImGui::Selectable(res->GetName().c_str(), chosen))
              {
                if (it == selected.end())
                {
                  selected.emplace_back(res);
                }
                else
                {
                  selected.erase(it);
                }
              }
            }

            ImGui::TreePop();
            ImGui::Separator();
          }
        }

        if (ImGui::Button("Close"))
        {
          m_b_imgui_multiple_choice_dialog_ = false;
          ImGui::End();
          return true;
        }

        ImGui::End();
      }
    }

    return false;
  }

  void       ResourceManager::OpenNewShaderDialog()
  {
    if (m_b_imgui_load_shader_dialog_)
    {
      if (ImGui::Begin("New Shader", &m_b_imgui_load_shader_dialog_, ImGuiWindowFlags_AlwaysAutoResize))
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

        static const char* topologies[] = 
        {
          "PointList", "LineList", "LineStrip", "TriangleList", "TriangleStrip", "LineListAdj", "LineStripAdj", "TriangleListAdj", "TriangleStripAdj", "PatchList"
        };

        int sel_domain          = 0;
        int sel_depth           = 0;
        int sel_depth_func      = 0;
        int sel_cull            = 0;
        int sel_fill            = 0;
        int sel_filter          = 0;
        int sel_sampler_address = 0;
        int sel_sampler_func    = 0;
        int sel_topology        = 0;

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
        ImGui::Combo("Topology", &sel_topology, topologies, IM_ARRAYSIZE(topologies));
        

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
               (UINT)(sel_sampler_address | sel_sampler_func >> 5),
               (D3D11_PRIMITIVE_TOPOLOGY)(sel_topology + 1)
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

  void ResourceManager::OpenNewAtlasDialog()
  {
    if (m_b_imgui_load_atlas_dialog_)
    {
      const std::string title = "New Atlas";

      if (ImGui::Begin(title.c_str(), &m_b_imgui_load_atlas_dialog_, ImGuiWindowFlags_AlwaysAutoResize))
      {
        static char atlas_name_buffer[256] = {};
        static char name_buffer[256] = {};
        static char tex_path_buffer[256] = {};
        static char xml_path_buffer[256] = {};

        static std::vector<std::tuple<std::string, std::string, std::string>> pair;

        ImGui::InputText("Atlas Name", atlas_name_buffer, 256);
        ImGui::InputText("Name", name_buffer, 256);
        ImGui::InputText("Texture Path", tex_path_buffer, 256);
        ImGui::InputText("XML Path", xml_path_buffer, 256);

        if (ImGui::Button("Add Texture"))
        {
          if (!std::strlen(xml_path_buffer))
          {
            const std::filesystem::path tex_path = tex_path_buffer;
            std::filesystem::path expected_xml = tex_path.filename();
            expected_xml.replace_extension(".xml");

            const std::filesystem::path               folder = tex_path.parent_path();
            const std::filesystem::directory_iterator it(folder);

            for (const auto& entry : it)
            {
              if (entry.path().filename() == expected_xml)
              {
                strcpy_s(xml_path_buffer, entry.path().generic_string().c_str());
                break;
              }
            }
          }

          if (!std::strlen(tex_path_buffer))
          {
            const std::filesystem::path xml_path = xml_path_buffer;
            std::filesystem::path expected_tex = xml_path.filename();
            expected_tex.replace_extension(".png");

            const std::filesystem::path               folder = xml_path.parent_path();
            const std::filesystem::directory_iterator it(folder);

            for (const auto& entry : it)
            {
              if (entry.path().filename() == expected_tex)
              {
                strcpy_s(tex_path_buffer, entry.path().generic_string().c_str());
                break;
              }
            }
          }

          if (std::filesystem::exists(tex_path_buffer) && std::filesystem::exists(xml_path_buffer))
          {
            pair.emplace_back(name_buffer, tex_path_buffer, xml_path_buffer);
            std::memset(name_buffer, 0, 256);
            std::memset(tex_path_buffer, 0, 256);
            std::memset(xml_path_buffer, 0, 256);
          }
        }

        static std::string buffer;
        ImGui::InputText("Folder", &buffer);

        if (ImGui::Button("Add multiples..."))
        {
          std::filesystem::path folder = buffer;
          const std::filesystem::recursive_directory_iterator it(folder);

          for (const auto& entry : it)
          {
            if (entry.path().extension() == ".xml")
            {
              std::filesystem::path tex_path = entry.path();
              tex_path.replace_extension(".png");

              if (std::filesystem::exists(tex_path))
              {
                pair.emplace_back
                  (
                   tex_path.stem().generic_string(),
                   tex_path.generic_string(),
                   entry.path().generic_string()
                  );
              }
            }
          }
        }

        if (ImGui::BeginChild("Texture List", ImVec2(0, 300)))
        {
          for (const auto& [name, tex_path, xml_path] : pair)
          {
            ImGui::Text("Name: %s", name.c_str());
            ImGui::Text("Texture Path: %s", tex_path.c_str());
            ImGui::Text("XML Path: %s", xml_path.c_str());
            ImGui::Separator();
          }

          ImGui::EndChild();
        }

        if (ImGui::Button("Load"))
        {
          try
          {
            std::vector<StrongTexture2D> textures;

            for (const auto& [name, tex_path, xml_path] : pair)
            {
              Resources::AtlasAnimation::Create(name, xml_path);
              textures.emplace_back(Resources::Texture2D::Create(name, tex_path, {}));
            }

            Resources::AtlasAnimationTexture::Create(atlas_name_buffer, "", textures);

            std::memset(name_buffer, 0, 256);
            std::memset(tex_path_buffer, 0, 256);
            std::memset(xml_path_buffer, 0, 256);
            m_b_imgui_load_atlas_dialog_ = false;
          }
          catch (const std::exception& e)
          {
            ImGui::SameLine();
            ImGui::Text(e.what());
            std::memset(name_buffer, 0, 256);
            std::memset(tex_path_buffer, 0, 256);
            std::memset(xml_path_buffer, 0, 256);
          }
        }

        if (ImGui::Button("Cancel"))
        {
          std::memset(name_buffer, 0, 256);
          std::memset(tex_path_buffer, 0, 256);
          std::memset(xml_path_buffer, 0, 256);
          m_b_imgui_load_atlas_dialog_ = false;
        }

        ImGui::End();
      }
    }
  }
}
