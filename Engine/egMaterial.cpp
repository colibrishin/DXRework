#include "pch.h"
#include "egMaterial.h"

#include "egDXCommon.h"
#include "egDXType.h"
#include "egReflectionEvaluator.h"
#include "egShape.h"
#include "egTexture.h"
#include "egType.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::Material,
 _ARTAG(_BSTSUPER(Resource))
 _ARTAG(m_material_cb_)
 _ARTAG(m_shaders_)
 _ARTAG(m_resources_)
)

namespace Engine::Resources
{
  Material::Material(const std::filesystem::path& path)
    : Resource(path, RES_T_MTR),
      m_material_cb_(),
      m_b_ignore_shader_(false),
      m_instance_count_(1)
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
    if (!m_b_ignore_shader_)
    {
      for (const auto& shd : m_shaders_loaded_) { shd->PreRender(dt); }
    }

    m_material_cb_.flags              = {};
    m_material_cb_.instance_count     = m_instance_count_;
    m_material_cb_.bone_texture_width = m_bone_count_;

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      // No need to render the all animation.
      if (type == RES_T_BONE_ANIM) { continue; }

      for (auto it = resources.begin(); it != resources.end(); ++it)
      {
        const auto res = *it;

        // flip the tex flag before rendering.
        if (type == RES_T_TEX)
        {
          const UINT idx                = std::distance(resources.begin(), it);
          m_material_cb_.flags.tex[idx] = 1;
        }

        res->PreRender(dt);
      }
    }

    GetRenderPipeline().SetMaterial(m_material_cb_);
  }

  void Material::Render(const float& dt)
  {
    if (!m_b_ignore_shader_)
    {
      for (const auto& shd : m_shaders_loaded_) { shd->Render(dt); }
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
          const UINT idx = std::distance(resources.begin(), it);
          res->GetSharedPtr<Texture>()->SetSlot(BIND_SLOT_TEX, idx);
        }
        else if(type == RES_T_SHAPE)
        {
          res->GetSharedPtr<Shape>()->SetInstanceCount(m_instance_count_);
        }

        res->Render(dt);
      }
    }
  }

  void Material::PostRender(const float& dt)
  {
    if (!m_b_ignore_shader_)
    {
      for (const auto& shd : m_shaders_loaded_) { shd->PostRender(dt); }
    }

    for (const auto& [type, resources] : m_resources_loaded_)
    {
      // No need to render the all animation.
      if (type == RES_T_BONE_ANIM) { continue; }

      for (const auto& res : resources) { res->PostRender(dt); }
    }

    GetRenderPipeline().SetMaterial({});
    m_instance_count_ = 1;
    m_b_ignore_shader_ = false;
  }

  void Material::OnDeserialized()
  {
    Resource::OnDeserialized();
    Load();
  }

  void Material::SetInstanceCount(UINT count)
  {
    m_instance_count_ = count;
  }

  void Material::IgnoreShader()
  {
    m_b_ignore_shader_ = true;
  }

  void Material::SetBoneCount(UINT count)
  {
    m_bone_count_ = count;
  }

  void Material::SetProperties(CBs::MaterialCB&& material_cb) noexcept { m_material_cb_ = std::move(material_cb); }

  void Material::SetTextureSlot(const std::string& name, const UINT slot)
  {
      auto& texs = m_resources_loaded_[which_resource<Texture>::value];
    const auto it   = std::ranges::find_if(texs, [&name](const StrongResource& res) { return res->GetName() == name; });

    if (it == texs.end()) { return; }
    if (const UINT idx = std::distance(texs.begin(), it); idx == slot) { return; }

    std::iter_swap(texs.begin() + slot, it);
  }

  Material::Material()
    : Resource("", RES_T_MTR),
      m_material_cb_(),
      m_b_ignore_shader_(false),
      m_instance_count_(1) {}

  void Material::Load_INTERNAL()
  {
    m_resources_loaded_.clear();
    m_shaders_loaded_.clear();

    for (const auto& name : m_shaders_)
    {
      if (const auto res = GetResourceManager().GetResource(name, RES_T_SHADER).lock())
      {
        m_shaders_loaded_.emplace_back(res->GetSharedPtr<Shader>());
      }
    }

    for (const auto& [type, names] : m_resources_)
    {
      for (const auto& name : names)
      {
        if (const auto res = GetResourceManager().GetResource(name, type).lock())
        {
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
