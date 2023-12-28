#include "pch.h"
#include "egMaterial.h"

namespace Engine::Resources
{
    Material::Material(const std::filesystem::path& path)
    : Resource(path, RES_T_MTR),
      m_material_cb_()
    {
        m_material_cb_.specular_power = 100.0f;
        m_material_cb_.specular_color = Vector4{0.5f, 0.5f, 0.5f, 0.5f};
        m_material_cb_.reflection_scale = 0.15f;
        m_material_cb_.refraction_scale = 0.15f;
        m_material_cb_.clip_plane = Vector4::Zero;
        m_material_cb_.reflection_translation = 0.5f;
    }

    void Material::PreUpdate(const float& dt) {}

    void Material::Update(const float& dt) {}

    void Material::PostUpdate(const float& dt) {}

    void Material::FixedUpdate(const float& dt) {}

    void Material::PreRender(const float& dt)
    {
        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->PreRender(dt);
        }
    }

    void Material::Render(const float& dt)
    {
        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->Render(dt);
        }
    }

    void Material::PostRender(const float& dt)
    {
        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->PostRender(dt);
        }
    }

    void Material::Load_INTERNAL()
    {
        for (const auto& [type, name] : m_resources_)
        {
            if (const auto res = GetResourceManager().GetResource(name, type).lock())
            {
                m_resources_loaded_[type] = res;
            }
        }
    }

    void Material::Unload_INTERNAL()
    {
        m_resources_loaded_.clear();
    }
}
