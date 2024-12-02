#pragma once
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

namespace Engine::Resources
{
	class AtlasAnimationTexture : public Texture3D
	{
	public:
		RESOURCE_T(RES_T_ATLAS_TEX)

		AtlasAnimationTexture(const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		eResourceType GetResourceType() const override;

		RESOURCE_SELF_INFER_GETTER_DECL(AtlasAnimationTexture)

		static boost::shared_ptr<AtlasAnimationTexture> Create(
			const std::string& name, const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases
		)
		{
			if (const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<AtlasAnimationTexture>(name).lock())
			{
				return ncheck;
			}

			if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<AtlasAnimationTexture>(path).lock())
			{
				return pcheck;
			}

			const auto obj = boost::make_shared<AtlasAnimationTexture>(path, atlases);
			Managers::ResourceManager::GetInstance().AddResource(name, obj);

			// Sort atlases by order of name
			std::ranges::sort
					(
					 obj->m_atlases_,
					 [](const Strong<Texture2D>& a, const Strong<Texture2D>& b)
					 {
						 return a->GetName() < b->GetName();
					 }
					);

			return obj;
		}

	protected:
		bool DoesWantMapByResource() const override;
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		bool map(const Weak<CommandPair>& w_cmd, ID3D12Resource* texture_resource) override;

	private:
		SERIALIZE_DECL

		AtlasAnimationTexture()
			: Texture3D("", {}) {}

		std::vector<Strong<Texture2D>> m_atlases_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AtlasAnimationTexture)
