#pragma once
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferDX12.hpp"

#include <map>

namespace Engine::Graphics::SBs 
{
	struct MATERIAL_API MaterialBindFlag
	{
		OffsetT<int> tex[CFG_PER_PARAM_BUFFER_SIZE];
		OffsetT<int> texArr[CFG_PER_PARAM_BUFFER_SIZE];
		OffsetT<int> texCube[CFG_PER_PARAM_BUFFER_SIZE];
		OffsetT<int> bone;
		OffsetT<int> atlas;
	};

	struct MATERIAL_API MaterialSB
	{
		SB_T(SB_TYPE_MATERIAL)
		MaterialBindFlag flags;

		float specularPower;
		float reflectionTranslation;
		float reflectionScale;
		float refractionScale;

		Color        overrideColor;
		Color        specularColor;
		Vector4      clipPlane;
		OffsetT<int> repeatTexture;
	};
}

namespace Engine::Resources
{
	class MATERIAL_API Material final : public Engine::Abstracts::Resource
	{
	public:
		struct TempParam
		{
			UINT          instanceCount = 1;
			bool          bypassShader  = false;
			eShaderDomain domain        = SHADER_DOMAIN_OPAQUE;
		};

		RESOURCE_T(RES_T_MTR);

		Material(const std::filesystem::path& path);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		bool IsRenderDomain(eShaderDomain domain) const noexcept;

		template <typename T>
		[[nodiscard]] auto GetResourcesByType() const
		{
			if (!m_resources_loaded_.contains(which_resource<T>::value))
			{
				return std::vector<Strong<Resource>>{};
			}
			return m_resources_loaded_.at(which_resource<T>::value);
		}

		[[nodiscard]] const std::map<const eResourceType, std::vector<Strong<Resource>>>& GetResources() const;

		template <typename T>
		[[nodiscard]] boost::weak_ptr<T> GetResource(const std::string& name) const
		{
			if (!m_resources_loaded_.contains(which_resource<T>::value))
			{
				return {};
			}
			if (m_resources_loaded_.at(which_resource<T>::value).empty())
			{
				return {};
			}

			const auto it = std::ranges::find(m_resources_loaded_.at(which_resource<T>::value), name);

			if (it == m_resources_loaded_.at(which_resource<T>::value).end())
			{
				return {};
			}

			const auto idx = std::distance(m_resources_loaded_.at(which_resource<T>::value).begin(), it);
			return boost::reinterpret_pointer_cast<T>(m_resources_loaded_.at(which_resource<T>::value)[idx]);
		}

		template <typename T>
		[[nodiscard]] boost::weak_ptr<T> GetResource(UINT idx) const
		{
			if (!m_resources_loaded_.contains(which_resource<T>::value))
			{
				return {};
			}
			if (m_resources_loaded_.at(which_resource<T>::value).empty())
			{
				return {};
			}
			if (m_resources_loaded_.at(which_resource<T>::value).size() <= idx)
			{
				return {};
			}

			const auto& anims = m_resources_loaded_.at(which_resource<T>::value);

			return boost::static_pointer_cast<T>(*(anims.begin() + idx));
		}

		void SetResource(const Strong<Resource>& resource);
		void SetTextureSlot(const std::string& name, UINT slot);

		[[nodiscard]] const Graphics::SBs::MaterialSB& GetMaterialSB() const;

		RESOURCE_SELF_INFER_GETTER_DECL(Material)
		RESOURCE_SELF_INFER_CREATE_DECL(Material)

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Material();

		Graphics::SBs::MaterialSB                   m_material_sb_;

		std::vector<std::pair<EntityName, MetadataPathStr>>                                m_shader_paths_;
		std::map<const eResourceType, std::vector<std::pair<EntityName, MetadataPathStr>>> m_resource_paths_;

		// non-serialized
		std::map<const eShaderDomain, Strong<Shader>>                m_shaders_loaded_;
		std::map<const eResourceType, std::vector<Strong<Resource>>> m_resources_loaded_;
	};
}

