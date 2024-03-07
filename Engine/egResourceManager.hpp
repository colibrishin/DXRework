#pragma once
#include <boost/type.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <imgui.h>

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
      if (!resource->GetMetadataPath().empty() && GetResourceByMetadataPath<T>(resource->GetMetadataPath()).lock())  { return; }
      if (!resource->GetPath().empty() && GetResourceByRawPath<T>(resource->GetPath()).lock()) { return; }

      m_resources_[which_resource<T>::value].insert(resource);
    }

    template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Abstract::Resource, T>>>
    void AddResource(const EntityName& name, const boost::shared_ptr<T>& resource)
    {
      if (!resource->GetMetadataPath().empty() && GetResourceByMetadataPath<T>(resource->GetMetadataPath()).lock()) { return; }
      if (!resource->GetPath().empty() && GetResourceByRawPath<T>(resource->GetPath()).lock()) { return; }

      m_resources_[which_resource<T>::value].insert(resource);
      resource->SetName(name);
    }

    template <typename T>
    boost::weak_ptr<T> GetResource(const EntityName& name)
    {
      auto& resources = m_resources_[which_resource<T>::value];
      auto  it        = std::find_if
        (
         resources.begin(), resources.end(), [&name](const StrongResource& resource)
         {
           return resource->GetName() == name;
         }
        );

      if (it != resources.end())
      {
        if (!(*it)->IsLoaded()) { (*it)->Load(); }

        return boost::reinterpret_pointer_cast<T>(*it);
      }

      return {};
    }

    WeakResource GetResource(const EntityName& name, const eResourceType& type)
    {
      auto&      resources = m_resources_[type];
      const auto it        = std::ranges::find_if
        (
         resources
         , [&name](const StrongResource& resource) { return resource->GetName() == name; }
        );

      if (it != resources.end())
      {
        if (!(*it)->IsLoaded()) { (*it)->Load(); }

        return *it;
      }

      return {};
    }

    template <typename T>
    boost::weak_ptr<T> GetResourceByRawPath(const std::filesystem::path& path)
    {
      if (path.empty()) { return {}; }

      auto& resources = m_resources_[which_resource<T>::value];
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

        return boost::reinterpret_pointer_cast<T>(*it);
      }

      return {};
    }

    template <typename T>
    boost::weak_ptr<T> GetResourceByMetadataPath(const std::filesystem::path& path)
    {
      if (path.empty()) { return {}; }

      auto& resources = m_resources_[which_resource<T>::value];
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

        return boost::reinterpret_pointer_cast<T>(*it);
      }
      else
      {
        if (std::filesystem::exists(path))
        {
          const auto res = Serializer::Deserialize<Entity>(path.generic_string())->GetSharedPtr<T>();
          m_resources_[which_resource<T>::value].insert(res);
          res->Load();
          return res;
        }
      }

      return {};
    }

    WeakResource GetResourceByRawPath(const std::filesystem::path& path, const eResourceType type);
    WeakResource GetResourceByMetadataPath(const std::filesystem::path& path, const eResourceType type);

    inline static bool m_b_imgui_load_dialog_[boost::mpl::size<LoadableResourceTypes>::value] = {false};

  private:
    friend struct SingletonDeleter;
    ~ResourceManager() override = default;

    void OpenNewShaderDialog();

    template <typename T, typename... Args>
    void OpenNewSimpleDialog(bool& flag, Args&&... args)
    {
      if (flag)
      {
        const std::string title = std::string("New ") + typeid(T).name();

        if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          static char name_buffer[256] = {};
          static char path_buffer[256] = {};

          ImGui::InputText("Name", name_buffer, 256);
          ImGui::InputText("Path", path_buffer, 256);

          if (ImGui::Button("Load"))
          {
            try
            {
              T::Create(name_buffer, path_buffer, std::forward<Args>(args)...);
              std::memset(name_buffer, 0, 256);
              std::memset(path_buffer, 0, 256);
              flag = false;
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
            std::memset(name_buffer, 0, 256);
            std::memset(path_buffer, 0, 256);
            flag = false;
          }

          ImGui::End();
        }
      }
    }

    bool m_b_imgui_load_texture_dialog_ = false;
    bool m_b_imgui_load_shape_dialog_   = false;
    bool m_b_imgui_load_sound_dialog_  = false;
    bool m_b_imgui_load_shader_dialog_  = false;
    bool m_b_imgui_load_font_dialog_   = false;

    std::map<eResourceType, std::set<StrongResource>> m_resources_;
    std::map<LocalResourceID, WeakResource>           m_resource_cache_;
    std::map<LocalResourceID, GlobalEntityID>         m_resource_ids_;
  };
} // namespace Engine::Manager
