#include "Material.h"
#include "Material.generated.h"

#include <algorithm>
#include <DirectXColors.h>

#include "SceneManager.h"

#include "ResourceManager.h"
#include "Texture.h"
#include "UIHelpersResourceManager.h"

#if CFG_RAYTRACING
#include "RaytracingShader.h"
#endif

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

			IUIAPI& ui = g_ui_accessor.GetInterface();

			*parent |= ui.NewLabelAndFloat(this, "SpecularPower", {"Specular Power", m_material_sb_.specularPower, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat(this, "ReflectionScale", {"Reflection Scale", m_material_sb_.reflectionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat(this, "RefractionScale", {"Refraction Scale", m_material_sb_.refractionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat(this, "ReflectionTranslation", {"Reflection Translation", m_material_sb_.reflectionTranslation, 0.1f, 0.f, 0.f, true});
			*parent |= ui.NewLabelAndVec4(this, "OverrideColor", {"Override Color", &m_material_sb_.overrideColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec4(this, "SpecularColor", {"Specular Color", &m_material_sb_.specularColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec3(this, "ClipPlane", {"Clip Plane", &m_material_sb_.clipPlane.x, 0.01f, 0.f, 0.f, true});
			*parent |= ui.NewCheckbox(this, "RepeatTexture", {"Repeat Texture", reinterpret_cast<bool&>(m_material_sb_.repeatTexture), true});

            *parent += ui.NewListBox( this, "TexturesListBox", { "Textures", 0, 0 } );
            for ( auto it = m_cached_textures_.begin(); it != m_cached_textures_.end(); ++it )
            {
                const size_t idx = std::distance( m_cached_textures_.begin(), it );
                if ( const Strong<Texture> &tex = it->lock() )
                {
                    *parent |= ui.NewSelectable( this,
                                                 std::format( "TextureSelectable{}", idx ),
                                                 { tex->GetName(), tex->m_ui_info_.dialogOpened } );
                    ( *parent |= ui.NewButton( this, std::format( "Texture{}MoveUp", idx ), { "Move Up" } ) )
                            .SetFunction( [ &, idx ]()
                            {
                                SwapTexture( idx, idx - 1 );
                            } );
                    ( *parent |= ui.NewButton( this, std::format( "Texture{}MoveDown", idx ), { "Move Down" } ) )
                            .SetFunction( [ &, idx ]()
                            {
                                SwapTexture( idx, idx + 1 );
                            } );
                    *parent |= ui.NewSeparator( this, std::format( "Texture{}Separator", idx ), {} );

                    if ( tex->m_ui_info_.dialogOpened )
                    {
                        if ( UIContext context = IUIAPI::NewContext(
                                ui.NewDialog( tex.get(),
                                              "TextureDialog",
                                              { tex->GetName(), tex->m_ui_info_.dialogOpened } ) ) )
                        {
                            tex->OnUIUpdate( &context, dt );
                        }
                    }
                }
                else
                {
                    *parent |= ui.NewText( this, "TextureEmptyText", { "Empty" } );
                    *parent |= ui.NewSeparator( this, "TextureSeparator", {} );
                }
			}
			--*parent;

			{
                static std::string shader_string = {};
                if ( const Strong<ShaderBase> &shader = m_cached_shader_.lock() )
                {
                    shader_string = shader->GetName();
                }
                else
                {
                    shader_string = {};
                }

                *parent |= ui.NewLabelAndText( this, "Shader", { "Shader", shader_string, false } );
                ( *parent |= ui.NewButton( this, "SetShaderButton", { "Set Shader" } ) ).SetFunction( [&]()
                {
                    m_ui_shader_dialog_ = !m_ui_shader_dialog_;
                } );
            }

			{
                static std::string atlas_string = {};
                if ( const Strong<AtlasAnimationTexture> &atlas = m_cached_atlas_.lock() )
                {
                    atlas_string = atlas->GetName();
                }
                else
                {
                    atlas_string = {};
                }

                *parent |= ui.NewLabelAndText( this, "AtlasTexuter", { "Atlas Texture", atlas_string, false } );
                ( *parent |= ui.NewButton( this, "AddTextureButton", { "Add Texture..." } ) ).SetFunction( [&]()
                {
                    m_ui_add_dialog_ = !m_ui_add_dialog_;
                } );
            }
			
			if (m_ui_shader_dialog_)
			{
				if (Weak<Resource> resource_to_load;
#if CFG_RAYTRACING
					UIHelpers::SingleResourceSelectionDialogInclusion<Material, Shader, RaytracingShader>(GetSharedPtr<Material>(), resource_to_load))
#else
				    UIHelpers::SingleResourceSelectionDialogInclusion<Material, Shader>(GetSharedPtr<Material>(), resource_to_load))
#endif
				    {
					if (const Strong<ShaderBase>& shader = Cast<ShaderBase>(resource_to_load))
					{
						SetShader(shader);
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
						if (const auto& atlas = Cast<AtlasAnimationTexture>(resource))
						{
							SetAtlasTexture(atlas);
							continue;
						}

						if (const Strong<Texture>& locked = Cast<Texture>(resource)) 
						{
							const auto& it = std::ranges::find_if(m_cached_textures_, [&locked](const Weak<Texture>& tex)
								{
									return tex.expired();
								});

							if (it != m_cached_textures_.end())
							{
								SetTexture(locked, std::distance(m_cached_textures_.begin(), it));
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

		if (const Strong<ShaderBase>& shader = m_cached_shader_.lock())
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

		if (!m_shader_path_.empty())
		{
			if (const auto& shader = ShaderBase::GetByMetadataPath(m_shader_path_).lock())
			{
				SetShader(shader);
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
				SetTexture(tex, i, false);
			}
		}

		Load();
	}

	void Material::SetTexture(const Weak<Texture>& texture, const size_t slot, const bool set_path)
	{
		if (slot >= m_cached_textures_.size()) 
		{
			return;
		}

		if (const Strong<Texture>& locked = texture.lock())
		{
			if (IsLoaded())
			{
				locked->Load();
				m_textures_[slot] = locked;
			}
			
			m_cached_textures_[slot] = locked;
			m_material_sb_.texEnabled[slot] = true;

			if (set_path)
			{
				m_texture_paths_[slot] = locked->GetMetadataPath();	
			}
		}
	}

	void Material::SwapTexture(const size_t before, const size_t after)
	{
		if (before >= m_cached_textures_.size() || after >= m_cached_textures_.size())
		{
			return;
		}

		if (const Strong<Texture>& locked = m_cached_textures_[before].lock())
		{
			if (IsLoaded())
			{
				std::swap(m_textures_[before], m_textures_[after]);
			}

			std::swap(m_cached_textures_[before], m_cached_textures_[after]);
		    std::swap(m_material_sb_.texEnabled[before], m_material_sb_.texEnabled[after]);
			std::swap(m_material_sb_.texSlot[before], m_material_sb_.texSlot[after]);
			std::swap(m_texture_paths_[before], m_texture_paths_[after]);
		}
	}

	void Material::SetAtlasTexture(const Weak<AtlasAnimationTexture>& texture)
	{
		if (const Strong<AtlasAnimationTexture>& locked = texture.lock())
		{
			if (IsLoaded())
			{
				locked->Load();
				m_atlas_ = locked;
			}
			
			m_cached_atlas_ = locked;
			m_atlas_path_ = locked->GetMetadataPath();
		}
	}

	void Material::SetShader(const Weak<ShaderBase>& shader)
	{
		if (const Strong<ShaderBase>& locked = shader.lock())
		{
			if (IsLoaded())
			{
				locked->Load();
				m_shader_ = locked;
			}
			
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

	Weak<ShaderBase> Material::GetShader() const
	{
		return m_cached_shader_;
	}

    Material::Material() : Resource("") {}

	void Material::Load_INTERNAL()
	{
		if (const Strong<AtlasAnimationTexture>& atlas = m_cached_atlas_.lock())
		{
			m_atlas_ = atlas;
			m_atlas_->Load();
		}

		if (const Strong<ShaderBase>& shader = m_cached_shader_.lock())
		{
			m_shader_ = shader;
			m_shader_->Load();
		}

		for (size_t i = 0; i < m_cached_textures_.size(); ++i)
		{
			if (const Strong<Texture>& tex = m_cached_textures_[i].lock())
			{
				m_textures_[i] = tex;
				m_textures_[i]->Load();
			}
		}
	}

	void Material::Unload_INTERNAL()
	{
		m_atlas_ = {};
		m_shader_ = {};
		m_textures_ = {};
	}
}
