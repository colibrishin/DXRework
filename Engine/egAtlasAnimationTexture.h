#pragma once
#include "egResource.h"
#include "egTexture2D.h"
#include "egTexture3D.h"

namespace Engine::Resources
{
	class AtlasAnimationTexture : public Texture3D
	{
	public:
		RESOURCE_T(RES_T_ATLAS_TEX)

		AtlasAnimationTexture(const std::filesystem::path& path, const std::vector<StrongTexture2D>& atlases);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		eResourceType GetResourceType() const override;

		RESOURCE_SELF_INFER_GETTER(AtlasAnimationTexture)

		static boost::shared_ptr<AtlasAnimationTexture> Create(
			const std::string& name, const std::filesystem::path& path, const std::vector<StrongTexture2D>& atlases
		)
		{
			if (const auto ncheck = GetResourceManager().GetResource<AtlasAnimationTexture>(name).lock())
			{
				return ncheck;
			}

			if (const auto pcheck = GetResourceManager().GetResourceByMetadataPath<AtlasAnimationTexture>(path).lock())
			{
				return pcheck;
			}

			const auto obj = boost::make_shared<AtlasAnimationTexture>(path, atlases);
			GetResourceManager().AddResource(name, obj);

			// Sort atlases by order of name
			std::ranges::sort
					(
					 obj->m_atlases_,
					 [](const StrongTexture2D& a, const StrongTexture2D& b)
					 {
						 return a->GetName() < b->GetName();
					 }
					);

			return obj;
		}

	protected:
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		bool map(char* mapped) override;

	private:
		SERIALIZE_DECL

		AtlasAnimationTexture()
			: Texture3D("", {}) {}

		std::vector<StrongTexture2D> m_atlases_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AtlasAnimationTexture)
