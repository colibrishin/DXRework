#include "../Public/Material.h"

#include <DirectXColors.h>

#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"
#include "Source/Runtime/Resources/AnimationTexture/Public/AnimationTexture.h"
#include "Source/Runtime/Resources/AtlasAnimationTexture/Public/AtlasAnimationTexture.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

RESOURCE_SELF_INFER_GETTER_IMPL(Engine::Resources::Material);

namespace Engine::Resources
{
	Material::Material(const std::filesystem::path& path)
		: Resource(path, RES_T_MTR),
		  m_material_sb_()
	{
		m_material_sb_.specularPower         = 100.0f;
		m_material_sb_.specularColor         = DirectX::Colors::White;
		m_material_sb_.reflectionScale       = 0.15f;
		m_material_sb_.refractionScale       = 0.15f;
		m_material_sb_.clipPlane             = Vector4::Zero;
		m_material_sb_.reflectionTranslation = 0.5f;
		m_material_sb_.repeatTexture         = false;
	}

	void Material::PreUpdate(const float& dt) {}

	void Material::Update(const float& dt) {}

	void Material::PostUpdate(const float& dt) {}

	void Material::FixedUpdate(const float& dt) {}

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

	const std::map<const eResourceType, std::vector<Strong<Abstracts::Resource>>>& Material::GetResources() const
	{
		return m_resources_loaded_;
	}

	void Material::SetTextureSlot(const std::string& name, const UINT slot)
	{
		auto       texs = m_resources_loaded_[which_resource<Texture>::value];
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
		: Resource("", RES_T_MTR),
		  m_material_sb_() {}

	void Material::SetResource(const Strong<Resource>& resource)
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
		     m_resource_paths_[resource->GetResourceType()],
		     [&resource](const std::pair<EntityName, MetadataPathStr>& pair)
		     {
			     return pair.second == resource->GetMetadataPath();
		     }
		    ) != m_resource_paths_[resource->GetResourceType()].end())
		{
			return;
		}

		m_resource_paths_[resource->GetResourceType()].emplace_back
				(resource->GetName(), resource->GetMetadataPath().string());
		m_resources_loaded_[resource->GetResourceType()].push_back(resource);


		if (resource->GetResourceType() == RES_T_BONE_ANIM)
		{
			m_material_sb_.flags.bone = 1;
		}

		if (resource->GetResourceType() == RES_T_ATLAS_ANIM)
		{
			m_material_sb_.flags.atlas = 1;
		}

		if (resource->GetResourceType() == RES_T_BONE_ANIM ||
			resource->GetResourceType() == RES_T_ATLAS_ANIM)
		{
			std::ranges::sort
			(
				m_resources_loaded_[resource->GetResourceType()],
				[](const Strong<Resource>& lhs, const Strong<Resource>& rhs)
				{
					return lhs->GetName() < rhs->GetName();
				}
			);
		}

		if (resource->GetResourceType() == RES_T_TEX)
		{
			// todo: distinguish tex type
			const auto& tex_arr = m_resources_loaded_[resource->GetResourceType()];
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
