#pragma once
#include <boost/serialization/access.hpp>

#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Abstracts/CoreResource/Public/Resource.h"
#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferDX12.hpp"

#include <map>

namespace Engine::Graphics::SBs 
{
	struct MaterialBindFlag
	{
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar& tex;
			ar& texArr;
			ar& texCube;
			ar& bone;
		}

		OffsetT<int> tex[PER_PARAM_BUFFER_SIZE];
		OffsetT<int> texArr[PER_PARAM_BUFFER_SIZE];
		OffsetT<int> texCube[PER_PARAM_BUFFER_SIZE];
		OffsetT<int> bone;
		OffsetT<int> atlas;
	};

	struct MaterialSB
	{
		SB_T(SB_TYPE_MATERIAL)

		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int file_version)
		{
			ar& flags;
			ar& specularPower;
			ar& reflectionTranslation;
			ar& reflectionScale;
			ar& refractionScale;
			ar& overrideColor;
			ar& specularColor;
			ar& clipPlane;
		}

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
	class Material final : public Engine::Abstracts::Resource
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

		void SetTempParam(TempParam&& param) noexcept;
		bool IsRenderDomain(eShaderDomain domain) const noexcept;

		template <typename T>
		[[nodiscard]] auto GetResources() const
		{
			if (!m_resources_loaded_.contains(which_resource<T>::value))
			{
				return std::vector<Strong<Resource>>{};
			}
			return m_resources_loaded_.at(which_resource<T>::value);
		}

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

		void SetTextureSlot(const std::string& name, UINT slot);

		void Draw(const float& dt, const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap);

		Graphics::StructuredBuffer<Graphics::SBs::MaterialSB>& GetMaterialSBBuffer();
		void                               UpdateMaterialSB(ID3D12GraphicsCommandList1* cmd);

		RESOURCE_SELF_INFER_GETTER_DECL(Material)
		RESOURCE_SELF_INFER_CREATE_DECL(Material)

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL
		Material();

		void SetResource(const Strong<Resource>& resource);

		Graphics::SBs::MaterialSB                   m_material_sb_;
		Graphics::StructuredBuffer<Graphics::SBs::MaterialSB> m_material_sb_data_;

		std::vector<std::pair<EntityName, MetadataPathStr>>                                m_shader_paths_;
		std::map<const eResourceType, std::vector<std::pair<EntityName, MetadataPathStr>>> m_resource_paths_;

		// non-serialized
		bool                                                       m_b_edit_dialog_;
		bool                                                       m_b_wait_for_choices_;
		std::vector<Strong<Resource>>                                m_resources_to_load_;
		TempParam                                                  m_temp_param_;
		std::map<const eShaderDomain, Strong<Shader>>                m_shaders_loaded_;
		std::map<const eResourceType, std::vector<Strong<Resource>>> m_resources_loaded_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Material)
