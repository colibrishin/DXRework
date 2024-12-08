#pragma once
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

namespace Engine::Resources
{
	class ATLASANIMATIONTEXTURE_API AtlasAnimationTexture : public Texture3D
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
		);

	protected:
		bool DoesWantMapByResource() const override;
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		bool map(const Weak<CommandPair>& w_cmd, ID3D12Resource* texture_resource) override;

	private:
		AtlasAnimationTexture()
			: Texture3D("", {}) {}

		std::vector<Strong<Texture2D>> m_atlases_;
	};
}
