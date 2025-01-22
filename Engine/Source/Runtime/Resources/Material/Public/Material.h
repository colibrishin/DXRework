#pragma once
#include "Source/Runtime/Core/StructuredBuffer/Public/StructuredBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"

#include <string>
#include <map>

#include "MaterialSB.h"

#include "Material.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_MATERIAL_API Material final : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Material(const std::filesystem::path& path);

		void OnUIUpdate(UIContext* const parent, const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		bool IsRenderDomain(eShaderDomain domain) const noexcept;

		template <typename T>
		[[nodiscard]] auto GetResourcesByType() const
		{
			if (!m_resources_loaded_.contains(T::StaticTypeHash()))
			{
				return std::vector<Strong<Resource>>{};
			}
			return m_resources_loaded_.at(T::StaticTypeHash());
		}

		[[nodiscard]] const std::map<ResourceType, std::vector<Strong<Resource>>>& GetResources() const;

		template <typename T>
		[[nodiscard]] Weak<T> GetResource(const std::string& name) const
		{
			if (!m_resources_loaded_.contains(T::StaticTypeHash()))
			{
				return {};
			}
			if (m_resources_loaded_.at(T::StaticTypeHash()).empty())
			{
				return {};
			}

			const auto it = std::ranges::find(m_resources_loaded_.at(T::StaticTypeHash()), name);

			if (it == m_resources_loaded_.at(T::StaticTypeHash()).end())
			{
				return {};
			}

			const auto idx = std::distance(m_resources_loaded_.at(T::StaticTypeHash()).begin(), it);
			return boost::reinterpret_pointer_cast<T>(m_resources_loaded_.at(T::StaticTypeHash())[idx]);
		}

		template <typename T>
		[[nodiscard]] Weak<T> GetResource(UINT idx) const
		{
			if (!m_resources_loaded_.contains(T::StaticTypeHash()))
			{
				return {};
			}
			if (m_resources_loaded_.at(T::StaticTypeHash()).empty())
			{
				return {};
			}
			if (m_resources_loaded_.at(T::StaticTypeHash()).size() <= idx)
			{
				return {};
			}

			const auto& anims = m_resources_loaded_.at(T::StaticTypeHash());

			return boost::static_pointer_cast<T>(*(anims.begin() + idx));
		}

		void SetResource(const Strong<Abstracts::Resource>& resource);
		void SetTextureSlot(const std::string& name, UINT slot);

		[[nodiscard]] const Graphics::SBs::MaterialSB& GetMaterialSB() const;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Material();

		EPROPERTY()
		Graphics::SBs::MaterialSB m_material_sb_;

#if WITH_EDITOR
		bool m_b_ui_edit_resource_ = false;
		bool m_b_ui_add_resource_ = false;

		void ProcessEditUI();
		void ProcessAddUI();
#endif

		EPROPERTY()
		std::vector<std::pair<EntityName, MetadataPathStr>>                         m_shader_paths_;
		
		EPROPERTY()
		std::map<ResourceType, std::vector<std::pair<EntityName, MetadataPathStr>>> m_resource_paths_;

		// non-serialized
		std::map<const eShaderDomain, Strong<Shader>>         m_shaders_loaded_;
		std::map<ResourceType, std::vector<Strong<Resource>>> m_resources_loaded_;
	};
}