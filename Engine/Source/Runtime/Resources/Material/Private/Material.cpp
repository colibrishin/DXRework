#include "../Public/Material.h"

#include <DirectXColors.h>

#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
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
		  m_material_sb_(),
		  m_b_edit_dialog_(false),
		  m_b_wait_for_choices_(false)
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

	void Material::SetTempParam(TempParam&& param) noexcept
	{
		m_temp_param_ = std::move(param);
	}

	bool Material::IsRenderDomain(eShaderDomain domain) const noexcept
	{
		return m_shaders_loaded_.contains(domain);
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

	void Material::UpdateMaterialSB(ID3D12GraphicsCommandList1* cmd)
	{
		if (m_resources_loaded_.contains(RES_T_BONE_ANIM))
		{
			m_material_sb_.flags.bone = 1;
		}

		if (m_resources_loaded_.contains(RES_T_ATLAS_ANIM))
		{
			m_material_sb_.flags.atlas = 1;
		}

		for (const auto& [type, resources] : m_resources_loaded_)
		{
			if (type == RES_T_SHAPE)
			{
				continue;
			}

			for (auto it = resources.begin(); it != resources.end(); ++it)
			{
				const auto res = *it;

				if (type == RES_T_TEX)
				{
					// todo: distinguish tex type
					const UINT idx                = static_cast<UINT>(std::distance(resources.begin(), it));
					m_material_sb_.flags.tex[idx] = 1;
				}
			}
		}

		// todo: Multiple same update for material
		m_material_sb_data_.SetData(cmd, 1, &m_material_sb_);
	}

	void Material::Draw(const float& dt, const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap)
	{
		const auto& cmd  = w_cmd.lock();
		const auto& heap = w_heap.lock();

		heap->BindGraphic(cmd->GetList4());

		if (!m_temp_param_.bypassShader)
		{
			cmd->GetList()->SetPipelineState(m_shaders_loaded_[m_temp_param_.domain]->GetPipelineState());
			cmd->GetList()->IASetPrimitiveTopology(m_shaders_loaded_[m_temp_param_.domain]->GetTopology());
			heap->SetSampler(m_shaders_loaded_[m_temp_param_.domain]->GetShaderHeap(), SAMPLER_TEXTURE);
		}

		if (!m_resources_loaded_.contains(RES_T_SHAPE))
		{
			return;
		}

		const auto& instance_count = m_temp_param_.instanceCount;

		if (m_resources_loaded_.contains(RES_T_BONE_ANIM))
		{
			m_material_sb_.flags.bone = 1;
		}

		if (m_resources_loaded_.contains(RES_T_ATLAS_ANIM))
		{
			m_material_sb_.flags.atlas = 1;
		}

		for (const auto& [type, resources] : m_resources_loaded_)
		{
			if (type == RES_T_SHAPE)
			{
				continue;
			}

			if (type == RES_T_ATLAS_TEX)
			{
				const auto& anim = resources.front()->GetSharedPtr<AtlasAnimationTexture>();
				anim->Bind(cmd, heap, BIND_TYPE_SRV, RESERVED_TEX_ATLAS, 0);
				continue;
			}

			for (auto it = resources.begin(); it != resources.end(); ++it)
			{
				const auto res = *it;

				if (type == RES_T_TEX)
				{
					// todo: distinguish tex type
					const UINT  idx = static_cast<UINT>(std::distance(resources.begin(), it));
					const auto& tex = res->GetSharedPtr<Texture>();

					tex->Bind(cmd, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, idx);

					m_material_sb_.flags.tex[idx] = 1;
				}
			}
		}

		// todo: Multiple same update for material
		m_material_sb_data_.SetData(cmd->GetList(), 1, &m_material_sb_);
		m_material_sb_data_.TransitionToSRV(cmd->GetList());
		m_material_sb_data_.CopySRVHeap(heap);
		Managers::RenderPipeline::GetInstance().BindConstantBuffers(cmd, heap);

		for (const auto& s : m_resources_loaded_[RES_T_SHAPE])
		{
			const auto& shape = s->GetSharedPtr<Shape>();

			if (const auto& anim = shape->GetAnimations().lock())
			{
				anim->Bind(cmd, heap, BIND_TYPE_SRV, RESERVED_TEX_BONES, 0);
			}

			for (const auto& mesh : shape->GetMeshes())
			{
				const auto& idx_count = mesh->GetIndexCount();
				const auto& vtx_view  = mesh->GetVertexView();
				const auto& idx_view  = mesh->GetIndexView();

				cmd->GetList()->IASetVertexBuffers(0, 1, &vtx_view);
				cmd->GetList()->IASetIndexBuffer(&idx_view);
				CheckSize<UINT>(idx_count, L"Warning: DrawIndexedInstance will takes large size of indices!");
				cmd->GetList()->DrawIndexedInstanced(static_cast<UINT>(idx_count), instance_count, 0, 0, 0);
			}

			if (const auto& anim = shape->GetAnimations().lock())
			{
				anim->Unbind(cmd, BIND_TYPE_SRV);
			}
		}

		if (m_resources_loaded_.contains(RES_T_TEX))
		{
			std::fill_n(m_material_sb_.flags.tex, m_resources_loaded_[RES_T_TEX].size(), 0);

			for (const auto& tex : m_resources_loaded_[RES_T_TEX])
			{
				tex->GetSharedPtr<Texture>()->Unbind(cmd, BIND_TYPE_SRV);
			}
		}

		if (m_resources_loaded_.contains(RES_T_ATLAS_TEX))
		{
			m_resources_loaded_[RES_T_ATLAS_TEX].front()->GetSharedPtr<Texture>()->Unbind(cmd, BIND_TYPE_SRV);
		}

		m_material_sb_data_.TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	Graphics::StructuredBuffer<Graphics::SBs::MaterialSB>& Material::GetMaterialSBBuffer()
	{
		return m_material_sb_data_;
	}

	Material::Material()
		: Resource("", RES_T_MTR),
		  m_material_sb_(),
		  m_b_edit_dialog_(false),
		  m_b_wait_for_choices_(false) {}

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

		std::ranges::sort
				(
				 m_resources_loaded_[resource->GetResourceType()],
				 [](const Strong<Resource>& lhs, const Strong<Resource>& rhs)
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
