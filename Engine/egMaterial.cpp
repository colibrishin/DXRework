#include "pch.h"
#include "egMaterial.h"

#include "egShader.hpp"
#include "egDXCommon.h"
#include "egDXType.h"
#include "egImGuiHeler.hpp"
#include "egShape.h"
#include "egTexture.h"
#include "egType.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::Material,
 _ARTAG(_BSTSUPER(Resource))
 _ARTAG(m_material_cb_)
 _ARTAG(m_shader_paths_)
 _ARTAG(m_resource_paths_)
)

namespace Engine::Resources
{
  Material::Material(const std::filesystem::path& path)
    : Resource(path, RES_T_MTR),
      m_material_cb_(),
      m_b_edit_dialog_(false)
  {
    m_material_cb_.specular_power         = 100.0f;
    m_material_cb_.specular_color         = DirectX::Colors::White;
    m_material_cb_.reflection_scale       = 0.15f;
    m_material_cb_.refraction_scale       = 0.15f;
    m_material_cb_.clip_plane             = Vector4::Zero;
    m_material_cb_.reflection_translation = 0.5f;
  }


  void Material::PreUpdate(const float& dt) {}

  void Material::Update(const float& dt) {}

  void Material::PostUpdate(const float& dt) {}

  void Material::FixedUpdate(const float& dt) {}

  void Material::PreRender(const float& dt)
  {
    if (!m_temp_param_.bypassShader)
    {
      m_shaders_loaded_[m_temp_param_.domain]->PreRender(dt);
    }

    m_material_cb_.flags = {};

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      // No need to render the all animation.
      if (type == RES_T_BONE_ANIM)
      {
        m_material_cb_.flags.bone = 1;
        continue;
      }

      for (auto it = resources.begin(); it != resources.end(); ++it)
      {
        const auto res = *it;

        if (type == RES_T_TEX)
        {
          const UINT idx                = static_cast<UINT>(std::distance(resources.begin(), it));
          m_material_cb_.flags.tex[idx] = 1;
        }

        res->PreRender(dt);
      }
    }

    GetRenderPipeline().SetMaterial(m_material_cb_);
  }

  void Material::Render(const float& dt)
  {
    if (!m_temp_param_.bypassShader)
    {
      m_shaders_loaded_[m_temp_param_.domain]->Render(dt);
    }

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      // No need to render the all animation.
      if (type == RES_T_BONE_ANIM) { continue; }

      for (auto it = resources.begin(); it != resources.end(); ++it)
      {
        const auto res = *it;

        if (type == RES_T_TEX)
        {
          // todo: distinguish tex type
          const UINT idx = static_cast<UINT>(std::distance(resources.begin(), it));
          res->GetSharedPtr<Texture>()->BindAs(D3D11_BIND_SHADER_RESOURCE, BIND_SLOT_TEX, idx, SHADER_PIXEL);
        }

        if (type == RES_T_SHAPE)
        {
          res->GetSharedPtr<Shape>()->SetInstanceCount(m_temp_param_.instanceCount);
        }

        res->Render(dt);
      }
    }
  }

  void Material::PostRender(const float& dt)
  {
    GetRenderPipeline().SetMaterial({});

    if (!m_temp_param_.bypassShader)
    {
      m_shaders_loaded_[m_temp_param_.domain]->PostRender(dt);
    }

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      // No need to render the all animation.
      if (type == RES_T_BONE_ANIM) { continue; }

      for (const auto& res : resources) { res->PostRender(dt); }
    }

    m_temp_param_ = {};
  }

  void Material::OnSerialized()
  {
    for (const auto& shader : m_shaders_loaded_ | std::views::values)
    {
      Serializer::Serialize(shader->GetName(), shader);
    }

    for (const auto& resources : m_resources_loaded_ | std::views::values)
    {
      for (const auto& resource : resources)
      {
        Serializer::Serialize(resource->GetName(), resource);
      }
    }
  }

  void Material::OnDeserialized()
  {
    Resource::OnDeserialized();
    Load();
  }

  void Material::OnImGui()
  {
    Resource::OnImGui();

    // Material properties
    FloatAligned("Specular Power", m_material_cb_.specular_power);
    FloatAligned("Reflection Scale", m_material_cb_.reflection_scale);
    FloatAligned("Refraction Scale", m_material_cb_.refraction_scale);
    FloatAligned("Reflection Translation", m_material_cb_.reflection_translation);

    ImGuiColorEditable("Override Color", GetID(), "override_color", m_material_cb_.override_color);
    ImGuiColorEditable("Specular Color", GetID(), "specular_color", m_material_cb_.specular_color);
    ImGuiVector3Editable("Clip plane", GetID(), "clip_plane", reinterpret_cast<Vector3&>(m_material_cb_.clip_plane));

    if (ImGui::Button("Edit Resources")) { m_b_edit_dialog_ = true; }

    if (m_b_edit_dialog_)
    {
        if (ImGui::Begin(GetName().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          if (ImGui::BeginListBox("Resource Used"))
          {
            for (auto& [type, resources] : m_resources_loaded_)
            {
              if (ImGui::TreeNode(g_resource_type_str[type]))
              {
                for (auto it = resources.begin(); it != resources.end();)
                {
                  if (ImGui::Selectable((*it)->GetName().c_str(), false))
                  {
                    std::erase_if
                      (
                       m_resource_paths_[(*it)->GetResourceType()], [&it](const std::pair<EntityName, MetadataPathStr>& pair)
                       {
                         return pair.first == (*it)->GetName() && pair.second == (*it)->GetMetadataPath();
                       }
                      );
                    it = resources.erase(it);
                  }
                  else { ++it; }
                }
                ImGui::TreePop();
              }
            }
            ImGui::EndListBox();
          }

        if (ImGui::BeginDragDropTarget() && ImGui::IsMouseReleased(0))
        {
          if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE", ImGuiDragDropFlags_AcceptBeforeDelivery))
          {
            if (const auto resource = static_cast<StrongResource*>(payload->Data))
            {
              SetResource(*resource);
            }
          }

          ImGui::EndDragDropTarget();
        }

        ImGui::End();
      }
    }
  }

  void Material::SetTempParam(TempParam&& param) noexcept { m_temp_param_ = std::move(param); }

  bool Material::IsRenderDomain(eShaderDomain domain) const noexcept { return m_shaders_loaded_.contains(domain); }

  void Material::SetProperties(CBs::MaterialCB&& material_cb) noexcept { m_material_cb_ = std::move(material_cb); }

  void Material::SetTextureSlot(const std::string& name, const UINT slot)
  {
    auto       texs = m_resources_loaded_[which_resource<Texture>::value];
    const auto it   = std::ranges::find_if(texs, [&name](const StrongResource& res) { return res->GetName() == name; });

    if (it == texs.end()) { return; }
    if (const UINT idx = static_cast<UINT>(std::distance(texs.begin(), it)); idx == slot) { return; }

    std::iter_swap(texs.begin() + slot, it);
  }

  Material::Material()
    : Resource("", RES_T_MTR),
      m_material_cb_(),
      m_b_edit_dialog_(false) {}

  void Material::SetResource(const StrongResource& resource)
  {
    if (resource->GetResourceType() == RES_T_MTR)
    {
      return;
    }

    if (resource->GetResourceType() == RES_T_MESH)
    {
      return;
    }

    if (!resource->IsLoaded())
    {
      resource->Load();
    }

    if (resource->GetResourceType() == RES_T_SHADER)
    {
      if (std::ranges::find_if(m_shader_paths_, [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
      {
        return pair.second == resource->GetMetadataPath();
      }) != m_shader_paths_.end())
      {
        return;
      }

      m_shader_paths_.emplace_back(resource->GetName(), resource->GetMetadataPath().string());
      m_shaders_loaded_[resource->GetSharedPtr<Shader>()->GetDomain()] = resource->GetSharedPtr<Shader>();
      return;
    }

    if (std::ranges::find_if
        (
         m_resource_paths_[resource->GetResourceType()],
         [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
         {
           return pair.second == resource->GetMetadataPath();
         }
        ) != m_resource_paths_[resource->GetResourceType()].end()) { return; }

    m_resource_paths_[resource->GetResourceType()].emplace_back(resource->GetName(), resource->GetMetadataPath().string());
    m_resources_loaded_[resource->GetResourceType()].push_back(resource);
  }

  void Material::Load_INTERNAL()
  {
    m_resources_loaded_.clear();
    m_shaders_loaded_.clear();

    for (const auto& [name, path] : m_shader_paths_)
    {
      const auto name_wise = GetResourceManager().GetResource<Shader>(name).lock();
      const auto path_wise = GetResourceManager().GetResourceByMetadataPath<Shader>(path).lock();

      if (name_wise || path_wise)
      {
        const auto& res = name_wise ? name_wise : path_wise;
        const auto shader = res->GetSharedPtr<Shader>();
        m_shaders_loaded_[shader->GetDomain()] = shader;
      }
    }

    for (const auto& [type, pairs] : m_resource_paths_)
    {
      for (const auto& [name, path] : pairs)
      {
        const auto path_wise = GetResourceManager().GetResourceByMetadataPath(path, type).lock();
        const auto name_wise = GetResourceManager().GetResource(name, type).lock();

        if (name_wise || path_wise)
        {
          const auto& res = name_wise ? name_wise : path_wise;
          m_resources_loaded_[type].push_back(res);
        }
      }
    }
  }

  void Material::Unload_INTERNAL()
  {
    m_resources_loaded_.clear();
    m_shaders_loaded_.clear();
  }
}
