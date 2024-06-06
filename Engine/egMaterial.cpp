#include "pch.h"
#include "egMaterial.h"

#include "egAnimationsTexture.h"
#include "egCommands.h"
#include "egDXCommon.h"
#include "egDXType.h"
#include "egImGuiHeler.hpp"
#include "egShader.hpp"
#include "egShape.h"
#include "egTexture.h"
#include "egType.h"

SERIALIZE_IMPL
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
      m_b_edit_dialog_(false),
      m_b_wait_for_choices_(false)
  {
    m_material_cb_.specularPower         = 100.0f;
    m_material_cb_.specularColor         = DirectX::Colors::White;
    m_material_cb_.reflectionScale       = 0.15f;
    m_material_cb_.refractionScale       = 0.15f;
    m_material_cb_.clipPlane             = Vector4::Zero;
    m_material_cb_.reflectionTranslation = 0.5f;
    m_material_cb_.repeatTexture         = false;
  }


  void Material::PreUpdate(const float& dt) {}

  void Material::Update(const float& dt) {}

  void Material::PostUpdate(const float& dt) {}

  void Material::FixedUpdate(const float& dt) {}

  void Material::PreRender(const float& dt) {}

  void Material::Render(const float& dt) {}

  void Material::PostRender(const float& dt) {}

  void Material::OnSerialized()
  {
    m_shader_paths_.clear();
    m_resource_paths_.clear();

    for (const auto& shader : m_shaders_loaded_ | std::views::values)
    {
      Serializer::Serialize(shader->GetName(), shader);
      m_shader_paths_.emplace_back(shader->GetName(), shader->GetMetadataPath().string());
    }

    for (const auto& resources : m_resources_loaded_ | std::views::values)
    {
      for (const auto& resource : resources)
      {
        Serializer::Serialize(resource->GetName(), resource);
        m_resource_paths_[resource->GetResourceType()].emplace_back(resource->GetName(), resource->GetMetadataPath().string());
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
    FloatAligned("Specular Power", m_material_cb_.specularPower);
    FloatAligned("Reflection Scale", m_material_cb_.reflectionScale);
    FloatAligned("Refraction Scale", m_material_cb_.refractionScale);
    FloatAligned("Reflection Translation", m_material_cb_.reflectionTranslation);

    ImGuiColorEditable("Override Color", GetID(), "override_color", m_material_cb_.overrideColor);
    ImGuiColorEditable("Specular Color", GetID(), "specular_color", m_material_cb_.specularColor);
    ImGuiVector3Editable("Clip plane", GetID(), "clip_plane", reinterpret_cast<Vector3&>(m_material_cb_.clipPlane));

    CheckboxAligned("Repeat Texture", reinterpret_cast<bool&>(m_material_cb_.repeatTexture.value));

    if (ImGui::Button("Edit Resources")) { m_b_edit_dialog_ = true; }

    if (m_b_edit_dialog_)
    {
      if (ImGui::Begin(GetName().c_str(), &m_b_edit_dialog_, ImGuiWindowFlags_AlwaysAutoResize))
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
                     m_resource_paths_[(*it)->GetResourceType()],
                     [&it](const std::pair<EntityName, MetadataPathStr>& pair)
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

          if (!m_shaders_loaded_.empty())
          {
            if (ImGui::TreeNode("Shader"))
            {
              for (auto it = m_shaders_loaded_.begin(); it != m_shaders_loaded_.end();)
              {
                if (ImGui::Selectable(it->second->GetName().c_str(), false))
                {
                  std::erase_if
                    (
                     m_shader_paths_,
                     [&it](const std::pair<EntityName, MetadataPathStr>& pair)
                     {
                       return pair.first == it->second->GetName() && pair.second == it->second->GetMetadataPath();
                     }
                    );
                  m_shaders_loaded_.erase(it);
                }
                else { ++it; }
              }

              ImGui::TreePop();
            }
          }

          ImGui::EndListBox();
        }

        if (ImGui::BeginDragDropTarget())
        {
          if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE"))
          {
            if (const auto resource = static_cast<StrongResource*>(payload->Data))
            {
              SetResource(*resource);
            }
          }

          ImGui::EndDragDropTarget();
        }

        if (ImGui::Button("Add multiple resources"))
        {
          if (GetResourceManager().RequestMultipleChoiceDialog())
          {
            m_b_wait_for_choices_ = true;
          }
        }

        if (m_b_wait_for_choices_ && ImGui::Begin("Add multiple resources..."))
        {
          if (GetResourceManager().OpenMultipleChoiceDialog(m_resources_to_load_))
          {
            for (const auto& resource : m_resources_to_load_)
            {
              SetResource(resource);
            }

            m_b_wait_for_choices_ = false;
          }

          ImGui::End();
        }

        ImGui::End();
      }
    }
  }

  void Material::SetTempParam(TempParam&& param) noexcept
  {
    m_temp_param_ = std::move(param);
  }

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

  void Material::Draw(const float& dt, const CommandPair& cmd, const DescriptorPtr& heap)
  {
    heap->BindGraphic(cmd);

    if (!m_temp_param_.bypassShader)
    {
      cmd.GetList()->SetPipelineState(m_shaders_loaded_[m_temp_param_.domain]->GetPipelineState());
      cmd.GetList()->IASetPrimitiveTopology(m_shaders_loaded_[m_temp_param_.domain]->GetTopology());
      heap->SetSampler(m_shaders_loaded_[m_temp_param_.domain]->GetShaderHeap(), SAMPLER_TEXTURE);
    }

    if (!m_resources_loaded_.contains(RES_T_SHAPE))
    {
      return;
    }

    const auto& instance_count = m_temp_param_.instanceCount;

    if (m_resources_loaded_.contains(RES_T_BONE_ANIM))
    {
      m_material_cb_.flags.bone = 1;
    }

    if (m_resources_loaded_.contains(RES_T_ATLAS_ANIM))
    {
      m_material_cb_.flags.atlas = 1;
    }

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      if (type == RES_T_SHAPE) { continue; }

      if (type == RES_T_ATLAS_ANIM)
      {
        const auto& anim = resources.front()->GetSharedPtr<AnimationsTexture>();
        anim->Bind(cmd, heap, BIND_TYPE_SRV, RESERVED_ATLAS, 0);
        continue;
      }

      for (auto it = resources.begin(); it != resources.end(); ++it)
      {
        const auto res = *it;

        if (type == RES_T_TEX)
        {
          // todo: distinguish tex type
          const UINT idx = static_cast<UINT>(std::distance(resources.begin(), it));
          const auto& tex = res->GetSharedPtr<Texture>();

          tex->Bind(cmd, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, idx);

          m_material_cb_.flags.tex[idx] = 1;
        }
      }
    }

    GetRenderPipeline().SetMaterial(m_material_cb_);
    GetRenderPipeline().UploadConstantBuffers(heap);

    for (const auto& s : m_resources_loaded_[RES_T_SHAPE])
    {
      const auto& shape = s->GetSharedPtr<Shape>();

      if (const auto& anim = shape->GetAnimations().lock())
      {
        anim->Bind(cmd, heap, BIND_TYPE_SRV, RESERVED_BONES, 0);
      }

      for (const auto& mesh: shape->GetMeshes())
      {
        const auto& idx_count = mesh->GetIndexCount();
        const auto& vtx_view = mesh->GetVertexView();
        const auto& idx_view = mesh->GetIndexView();

        cmd.GetList()->IASetVertexBuffers(0, 1, &vtx_view);
        cmd.GetList()->IASetIndexBuffer(&idx_view);
        cmd.GetList()->DrawIndexedInstanced(idx_count, instance_count, 0, 0, 0);
      }

      if (const auto& anim = shape->GetAnimations().lock())
      {
        anim->Unbind(cmd, BIND_TYPE_SRV);
      }
    }

    if (m_resources_loaded_.contains(RES_T_TEX))
    {
      std::fill_n(m_material_cb_.flags.tex, m_resources_loaded_[RES_T_TEX].size(), 0);

      for (const auto& tex : m_resources_loaded_[RES_T_TEX])
      {
        tex->GetSharedPtr<Texture>()->Unbind(cmd, BIND_TYPE_SRV);
      }
    }
  }

  Material::Material()
    : Resource("", RES_T_MTR),
      m_material_cb_(),
      m_b_edit_dialog_(false),
      m_b_wait_for_choices_(false) {}

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
      if (!resource->GetMetadataPath().empty() && 
          std::ranges::find_if(m_shader_paths_, [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
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

    if (!resource->GetMetadataPath().empty() &&
        std::ranges::find_if
        (
         m_resource_paths_[resource->GetResourceType()],
         [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
         {
           return pair.second == resource->GetMetadataPath();
         }
        ) != m_resource_paths_[resource->GetResourceType()].end())
    {
      return;
    }

    m_resource_paths_[resource->GetResourceType()].emplace_back(resource->GetName(), resource->GetMetadataPath().string());
    m_resources_loaded_[resource->GetResourceType()].push_back(resource);

    std::ranges::sort
      (
       m_resources_loaded_[resource->GetResourceType()],
       [](const StrongResource& lhs, const StrongResource& rhs)
       {
         return lhs->GetName() < rhs->GetName();
       }
      );
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
