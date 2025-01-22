#include "../Public/Material.h"

#include <DirectXColors.h>

#include "ModuleManager/Public/ModuleManager.h"

#include "SceneManager/Public/SceneManager.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine::Resources
{
	Material::Material(const std::filesystem::path& path)
		: Resource(path),
		  m_material_sb_()
	{
		m_material_sb_.specularPower         = 100.0f;
		m_material_sb_.specularColor         = Color{1.f, 1.f, 1.f, 1.f};
		m_material_sb_.reflectionScale       = 0.15f;
		m_material_sb_.refractionScale       = 0.15f;
		m_material_sb_.clipPlane             = Vector4{0.f, 0.f, 0.f ,0.f};
		m_material_sb_.reflectionTranslation = 0.5f;
		m_material_sb_.repeatTexture         = false;
	}

	void Material::OnUIUpdate(UIContext* const parent, const float dt)
	{
#if WITH_EDITOR
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
			*parent |= ui.NewCheckbox({"Repeat Texture", reinterpret_cast<bool&>(m_material_sb_.repeatTexture.value)});

			(*parent |= ui.NewButton({"Edit Resources"})).SetFunction([&]()
			{
				m_b_ui_edit_resource_ = !m_b_ui_edit_resource_;
			});

			ProcessEditUI();

			(*parent |= ui.NewButton({"Add Resources"})).SetFunction([&]()
			{
				m_b_ui_add_resource_ = !m_b_ui_add_resource_;

				if (m_b_ui_add_resource_)
				{
					if (!Managers::ResourceManager::GetInstance().RequestAddResourceDialog())
					{
						m_b_ui_add_resource_ = false;
					}
				}
				else
				{
					Managers::ResourceManager::GetInstance().EndAddResourceDialog();
				}
			});

			ProcessAddUI();
		}
#endif
	}

#if WITH_EDITOR
	void Material::ProcessEditUI()
	{
		if (m_b_ui_edit_resource_)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			if (UIContext context = UIInterface::NewContext(ui.NewDialog({this, "Edit Resources", m_b_ui_edit_resource_})))
			{
				context += ui.NewListBox({"Resource Used", -1, -1});
				context >> ui.NewDragAndDropTarget({"RESOURCE", [&](void* ptr)
				{
					if (auto casted = static_cast<Strong<Abstracts::Resource>*>(ptr))
					{
						SetResource(*casted);
					}
				}});

				for (auto& resources : m_resources_loaded_ | std::views::values)
				{
					if (resources.empty())
					{
						continue;
					}

					const std::string_view type_name = (*resources.begin())->GetPrettyTypeName();
					context += ui.NewTreeNode({type_name});

					for (auto it = resources.begin(); it != resources.end(); ++it)
					{
						bool temp = false;
						GlobalEntityID target_id = (*it)->GetID();

						(context |= ui.NewSelectable({(*it)->GetName(), temp})).SetFunction([&, target_id]()
						{
							std::erase_if(resources, [target_id](const Strong<Resource>& value)
							{
								return value->GetID() == target_id;
							});
						});
					}

					--context;
				}

				--context;
			}
		}
	}

	void Material::ProcessAddUI()
	{
		if (m_b_ui_add_resource_)
		{
			std::vector<Strong<Resource>> resource_to_load{};

			if (Managers::ResourceManager::GetInstance().TryAddResourceDialog(resource_to_load))
			{
				m_b_ui_add_resource_ = false;

				for (Strong<Resource>& resource : resource_to_load)
				{
					/*
					if (resource == GetSharedPtr<Material>())
					{
						continue;
					}*/

					if (resource->IsBaseOf(Material::StaticTypeHash()))
					{
						continue;
					}

					if (resource->IsBaseOf(Mesh::StaticTypeHash()))
					{
						continue;
					}

					SetResource(resource);
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
	}

	void Material::OnDeserialized()
	{
		Resource::OnDeserialized();
		Load();
	}

	bool Material::IsRenderDomain(eShaderDomain domain) const noexcept
	{
		return m_shaders_loaded_.contains(domain);
	}

	const std::map<ResourceType, std::vector<Strong<Abstracts::Resource>>>& Material::GetResources() const
	{
		return m_resources_loaded_;
	}

	void Material::SetTextureSlot(const std::string& name, const UINT slot)
	{
		auto       texs = m_resources_loaded_[Texture::StaticTypeHash()];
		const auto it   = std::ranges::find_if
				(
				 texs, [&name](const Strong<Resource>& res)
				 {
					 return res->GetName() == name;
				 }
				);

		if (it == texs.end())
		{
			return;
		}
		if (const UINT idx = static_cast<UINT>(std::distance(texs.begin(), it));
			idx == slot)
		{
			return;
		}

		std::iter_swap(texs.begin() + slot, it);
	}

	const Graphics::SBs::MaterialSB& Material::GetMaterialSB() const
	{
		return m_material_sb_;
	}

	Material::Material()
		: Resource(""),
		  m_material_sb_() {}

	void Material::SetResource(const Strong<Resource>& resource)
	{
		if (resource->IsBaseOf(Material::StaticTypeHash()))
		{
			return;
		}

		if (resource->IsBaseOf(Mesh::StaticTypeHash()))
		{
			return;
		}

		if (!resource->IsLoaded())
		{
			resource->Load();
		}

		if (resource->IsBaseOf(Shader::StaticTypeHash()))
		{
			if (!resource->GetMetadataPath().empty() &&
			    std::ranges::find_if
			    (
			     m_shader_paths_, [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
			     {
				     return pair.second == resource->GetMetadataPath();
			     }
			    ) != m_shader_paths_.end())
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
		     m_resource_paths_[resource->GetTypeHash()],
		     [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
		     {
			     return pair.second == resource->GetMetadataPath();
		     }
		    ) != m_resource_paths_[resource->GetTypeHash()].end())
		{
			return;
		}

		m_resource_paths_[resource->GetTypeHash()].emplace_back
				(resource->GetName(), resource->GetMetadataPath().string());
		m_resources_loaded_[resource->GetTypeHash()].push_back(resource);


		if (resource->IsBaseOf(BoneAnimation::StaticTypeHash()))
		{
			m_material_sb_.flags.bone = 1;
		}

		if (resource->IsBaseOf(AtlasAnimation::StaticTypeHash()))
		{
			m_material_sb_.flags.atlas = 1;
		}

		if (resource->IsBaseOf(BoneAnimation::StaticTypeHash()) ||
			resource->IsBaseOf(AtlasAnimation::StaticTypeHash()))
		{
			std::ranges::sort
			(
				m_resources_loaded_[resource->GetTypeHash()],
				[](const Strong<Resource>& lhs, const Strong<Resource>& rhs)
				{
					return lhs->GetName() < rhs->GetName();
				}
			);
		}

		if (resource->IsBaseOf(Texture::StaticTypeHash()))
		{
			// todo: distinguish tex type
			const auto& tex_arr = m_resources_loaded_[resource->GetTypeHash()];
			decltype(m_resources_loaded_)::mapped_type::const_iterator it = std::find_if(tex_arr.begin(), tex_arr.end(), [&resource](const Strong<Resource>& other)
				{
					return resource == other;
				});
			const UINT idx = static_cast<UINT>(std::distance(tex_arr.begin(), it));
			m_material_sb_.flags.tex[idx] = 1;
		}
	}

	void Material::Load_INTERNAL()
	{
		m_resources_loaded_.clear();
		m_shaders_loaded_.clear();

		for (const auto& [name, path] : m_shader_paths_)
		{
			const auto name_wise = Managers::ResourceManager::GetInstance().GetResource<Shader>(name).lock();
			const auto path_wise = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Shader>(path).lock();

			if (name_wise || path_wise)
			{
				const auto& res                        = name_wise ? name_wise : path_wise;
				const auto  shader                     = res->GetSharedPtr<Shader>();
				m_shaders_loaded_[shader->GetDomain()] = shader;
			}
		}

		for (const auto& [type, pairs] : m_resource_paths_)
		{
			for (const auto& [name, path] : pairs)
			{
				const auto path_wise = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath(path, type).lock();
				const auto name_wise = Managers::ResourceManager::GetInstance().GetResource(name, type).lock();

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