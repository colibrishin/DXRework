#include "pch.h"
#include "egMaterial.h"
#include "egType.h"

namespace Engine::Resources
{
    Material::Material(const std::filesystem::path& path)
    : Resource(path, RES_T_MTR),
      m_material_cb_()
    {
        m_material_cb_.specular_power = 100.0f;
        m_material_cb_.specular_color = DirectX::Colors::White;
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
        if (!ShaderCheck())
        {
            return;
        }

        GetRenderPipeline().SetMaterial(m_material_cb_);

        for (const auto& shd : m_shaders_loaded_ | std::views::values)
        {
	        shd->PreRender(dt);
        }

        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->PreRender(dt);
        }
    }

    void Material::Render(const float& dt)
    {
        if (!ShaderCheck())
        {
            return;
        }

        for (const auto& shd : m_shaders_loaded_ | std::views::values)
        {
        	shd->Render(dt);
		}

        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->Render(dt);
        }
    }

    void Material::PostRender(const float& dt)
    {
        if (!ShaderCheck())
        {
            return;
        }

        GetRenderPipeline().SetMaterial({});

        for (const auto& shd : m_shaders_loaded_ | std::views::values)
        {
        	shd->PostRender(dt);
		}

        for (const auto& res : m_resources_loaded_ | std::views::values)
        {
            res->PostRender(dt);
        }
    }

    void Material::SetProperties(CBs::MaterialCB&& material_cb) noexcept
    {
        m_material_cb_ = std::move(material_cb);
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

    bool Material::ShaderCheck() const
    {
        if (!m_shaders_loaded_.contains(convert_shaderT_enum<VertexShader>::value_e()))
        {
            GetDebugger().Log(L"Vertex shader not loaded for material");
            return false;
        }
        if (!m_shaders_loaded_.contains(convert_shaderT_enum<PixelShader>::value_e()))
        {
            GetDebugger().Log(L"Pixel shader not loaded for material");
            return false;
        }

        return true;
    }
}
