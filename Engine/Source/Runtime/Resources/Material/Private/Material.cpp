#include "../Public/Material.h"
#include "Material.generated.h"

#include <algorithm>
#include <DirectXColors.h>

#include "ModuleManager/Public/ModuleManager.h"

#include "SceneManager/Public/SceneManager.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "UIHelpersResourceManager.h"

namespace Engine::Resources
{
	Material::Material(const Graphics::MaterialPrimitive& material) :
	Resource(""),
	m_material_sb_(material) {}

#if WITH_EDITOR
	void Material::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			Resource::OnUIUpdate(parent, dt);

			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			*parent |= ui.NewLabelAndFloat({"Specular Power", m_material_sb_.specularPower, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Reflection Scale", m_material_sb_.reflectionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Refraction Scale", m_material_sb_.refractionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Reflection Translation", m_material_sb_.reflectionTranslation, 0.1f, 0.f, 0.f, true});
			*parent |= ui.NewLabelAndVec4({"Override Color", &m_material_sb_.overrideColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec4({"Specular Color", &m_material_sb_.specularColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec3({"Clip Plane", &m_material_sb_.clipPlane.x, 0.01f, 0.f, 0.f, true});
			*parent |= ui.NewCheckbox({"Repeat Texture", reinterpret_cast<bool&>(m_material_sb_.repeatTexture)});

			*parent += ui.NewListBox({ "Textures", 0, 0 });
			for (auto it = m_textures_.begin(); it != m_textures_.end(); ++it)
			{
				if (*it == nullptr)
				{
					continue;
				}

				const Strong<Texture>& tex = *it;

				*parent |= ui.NewSelectable({ tex->GetName(), tex->m_ui_info_.dialogOpened });
				(*parent |= ui.NewButton({ "Move Up" })).SetFunction([&]()
					{
						SetTexture(*it, std::distance(m_textures_.begin(), it) - 1);
					});
				(*parent |= ui.NewButton({ "Move Down" })).SetFunction([&]()
					{
						SetTexture(*it, std::distance(m_textures_.begin(), it) + 1);
					});
				*parent |= ui.NewSeparator({});

				if (tex->m_ui_info_.dialogOpened) 
				{
					if (UIContext context = UIInterface::NewContext(ui.NewDialog({ tex.get(), tex->GetName(), tex->m_ui_info_.dialogOpened })))
					{
						tex->OnUIUpdate(&context, dt);
					}
				}
			}
			--*parent;
			
			static std::string empty_string = {};
			*parent |= ui.NewLabelAndText({ "Atlas Texture", m_atlas_loaded_ ? const_cast<std::string&>(m_atlas_loaded_->GetName()) : empty_string, false});

			(*parent |= ui.NewButton({ "Add Texture..." })).SetFunction([&]()
				{
					m_ui_add_dialog_ = !m_ui_add_dialog_;
				});

			if (m_ui_add_dialog_)
			{
				if (std::vector<Weak<Abstracts::Resource>> resource_to_load;
					UIHelpers::MultipleResourceSelectionDialogInclusion<Material, Texture>(GetSharedPtr<Material>(), resource_to_load))
				{
					for (const Weak<Resource>& resource : resource_to_load)
					{
						if (const Strong<Resource>& locked = resource.lock())
						{
							if (locked->GetTypeHash()->IsDerivedOf(AtlasAnimationTexture::StaticTypeHash()))
							{
								SetAtlasTexture(locked->GetSharedPtr<AtlasAnimationTexture>());
								continue;
							}

							const auto& it = std::find(m_textures_.begin(), m_textures_.end(), nullptr);

							if (it != m_textures_.end())
							{
								SetTexture(locked->GetSharedPtr<Texture>(), std::distance(m_textures_.begin(), it));
							}
						}
					}

					m_ui_add_dialog_ = false;
				}
			}
		}
	}
#endif

	void Material::PreUpdate(const float dt) {}

	void Material::Update(const float dt) {}

	void Material::PostUpdate(const float dt) {}

	void Material::FixedUpdate(const float dt) {}

	void Material::OnSerialized()
	{
		Resource::OnSerialized();
		Load();
	}

	void Material::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	void Material::SetTexture(const Weak<Texture>& texture, const size_t slot)
	{
		if (slot > m_textures_.size()) 
		{
			return;
		}

		if (const Strong<Texture>& locked = texture.lock())
		{
			if (const auto it = std::find(m_textures_.begin(), m_textures_.end(), locked);
				it != m_textures_.end())
			{
				const size_t idx = std::distance(m_textures_.begin(), it);
				m_textures_[idx] = {};
				m_material_sb_.texSlot[slot] = false;
				m_texture_paths_[slot] = "";
			}

			m_textures_[slot] = locked;
			m_material_sb_.texSlot[slot] = true;
			m_texture_paths_[slot] = locked->GetMetadataPath();
		}
	}

	void Material::SetAtlasTexture(const Weak<AtlasAnimationTexture>& texture)
	{
		if (const Strong<AtlasAnimationTexture>& locked = texture.lock())
		{
			m_atlas_loaded_ = locked;
			m_atlas_path_ = locked->GetMetadataPath();
		}
	}

	void Material::SetShader(const Weak<Shader>& shader)
	{
		if (const Strong<Shader>& locked = shader.lock())
		{
			m_shader_ = locked;
			m_shader_path_ = locked->GetMetadataPath();
		}
	}

	const Graphics::MaterialPrimitive& Material::GetPrimitive() const
	{
		return m_material_sb_;
	}

	const Material::TextureArray& Material::GetTextures() const
	{
		return m_textures_;
	}

	Weak<AtlasAnimationTexture> Material::GetAtlasTexture() const
	{
		return m_atlas_loaded_;
	}

	Weak<AtlasAnimation> Material::GetAtlasAnimation(const size_t idx) const
	{
		if (m_atlas_loaded_)
		{
			return m_atlas_loaded_->GetAnimation(idx);
		}

		return {};
	}

	Weak<Shader> Material::GetShader() const
	{
		return m_shader_;
	}

	Material::Material()
		: Resource("") {}

	void Material::Load_INTERNAL() {}

	void Material::Unload_INTERNAL() {}
}
