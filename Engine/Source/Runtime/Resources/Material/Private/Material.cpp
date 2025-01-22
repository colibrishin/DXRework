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
			for (auto it = m_cached_textures_.begin(); it != m_cached_textures_.end(); ++it)
			{
				if (const Strong<Texture>& tex = it->lock())
				{
					*parent |= ui.NewSelectable({ tex->GetName(), tex->m_ui_info_.dialogOpened });
					(*parent |= ui.NewButton({ "Move Up" })).SetFunction([&]()
						{
							SetTexture(*it, std::distance(m_cached_textures_.begin(), it) - 1);
						});
					(*parent |= ui.NewButton({ "Move Down" })).SetFunction([&]()
						{
							SetTexture(*it, std::distance(m_cached_textures_.begin(), it) + 1);
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
			}
			--*parent;

			{
				std::string shader_string = {};
				if (const Strong<Shader>& shader = m_cached_shader_.lock())
				{
					shader_string = shader->GetName();
				}
			
				*parent |= ui.NewLabelAndText({"Shader", shader_string, false});
				(*parent |= ui.NewButton({"Set Shader"})).SetFunction([&]()
					{
						m_ui_shader_dialog_ = !m_ui_shader_dialog_;
					});
			}

			{
				std::string atlas_string = {};
				if (const Strong<AtlasAnimationTexture>& atlas = m_cached_atlas_.lock())
				{
					atlas_string = atlas->GetName();
				}
				*parent |= ui.NewLabelAndText({ "Atlas Texture", atlas_string, false});
				(*parent |= ui.NewButton({ "Add Texture..." })).SetFunction([&]()
					{
						m_ui_add_dialog_ = !m_ui_add_dialog_;
					});
			}
			
			if (m_ui_shader_dialog_)
			{
				if (Weak<Resource> resource_to_load;
					UIHelpers::SingleResourceSelectionDialogInclusion<Material, Shader>(GetSharedPtr<Material>(), resource_to_load))
				{
					if (const Strong<Resource>& locked = resource_to_load.lock())
					{
						SetShader(locked->GetSharedPtr<Shader>());
					}

					m_ui_shader_dialog_ = false;
				}
			}
			
			if (m_ui_add_dialog_)
			{
				if (std::vector<Weak<Resource>> resource_to_load;
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

		if (const Strong<Shader>& shader = m_cached_shader_.lock())
		{
			Serializer::Serialize(shader->GetName(), shader);
			m_shader_path_ = shader->GetMetadataPath();
		}

		if (const Strong<AtlasAnimationTexture>& atlas = m_cached_atlas_.lock())
		{
			Serializer::Serialize(atlas->GetName(), atlas);
			m_atlas_path_ = atlas->GetMetadataPath();
		}

		for (auto it = m_cached_textures_.begin(); it != m_cached_textures_.end(); ++it)
		{
			if (const Strong<Texture>& tex = it->lock())
			{
				Serializer::Serialize(tex->GetName(), tex);
				m_texture_paths_[std::distance(m_cached_textures_.begin(), it)] = tex->GetMetadataPath();
			}
		}
	}

	void Material::OnDeserialized()
	{
		Resource::OnDeserialized();
		Load();
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
				m_cached_textures_[idx] = {};
				m_material_sb_.texSlot[slot] = false;
				m_texture_paths_[slot] = "";
			}

			m_textures_[slot] = locked;
			m_cached_textures_[slot] = locked;
			m_material_sb_.texSlot[slot] = true;
			m_texture_paths_[slot] = locked->GetMetadataPath();
		}
	}

	void Material::SetAtlasTexture(const Weak<AtlasAnimationTexture>& texture)
	{
		if (const Strong<AtlasAnimationTexture>& locked = texture.lock())
		{
			m_atlas_ = locked;
			m_cached_atlas_ = locked;
			m_atlas_path_ = locked->GetMetadataPath();
		}
	}

	void Material::SetShader(const Weak<Shader>& shader)
	{
		if (const Strong<Shader>& locked = shader.lock())
		{
			m_shader_ = locked;
			m_cached_shader_ = locked;
			m_shader_path_ = locked->GetMetadataPath();
		}
	}

	const MaterialPrimitive& Material::GetPrimitive() const
	{
		return m_material_sb_;
	}

	const Material::WeakTextureArray& Material::GetTextures() const
	{
		return m_cached_textures_;
	}

	Weak<AtlasAnimationTexture> Material::GetAtlasTexture() const
	{
		return m_cached_atlas_;
	}

	Weak<AtlasAnimation> Material::GetAtlasAnimation(const size_t idx) const
	{
		if (const Strong<AtlasAnimationTexture>& anim = m_cached_atlas_.lock())
		{
			return anim->GetAnimation(idx);
		}

		return {};
	}

	Weak<Shader> Material::GetShader() const
	{
		return m_shader_;
	}

	Material::Material() : Resource("") {}

	void Material::Load_INTERNAL()
	{
		if (!m_shader_path_.empty())
		{
			if (const auto& shader = Shader::GetByMetadataPath(m_shader_path_).lock())
			{
				SetShader(m_shader_);
			}
		}

		if (!m_atlas_path_.empty())
		{
			if (const auto& atlas = AtlasAnimationTexture::GetByMetadataPath(m_atlas_path_).lock())
			{
				SetAtlasTexture(atlas);
			}
		}

		for (size_t i = 0; i < m_texture_paths_.size(); ++i)
		{
			if (const auto& tex = Texture::GetByMetadataPath(m_texture_paths_[i]).lock())
			{
				SetTexture(tex, i);
			}
		}
	}

	void Material::Unload_INTERNAL() {}
}
