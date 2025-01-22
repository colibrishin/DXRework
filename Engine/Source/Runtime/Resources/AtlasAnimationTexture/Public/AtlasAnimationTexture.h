#pragma once
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"
#include "AtlasAnimation.h"

#include "AtlasAnimationTexture.generated.h"

namespace Engine::Resources
{
	ECLASS(resource)
	class ENGINE_ATLASANIMATIONTEXTURE_API AtlasAnimationTexture : public Texture3D
	{
		GENERATE_BODY
	public:
		AtlasAnimationTexture(
			const std::filesystem::path& path, 
			const std::vector<Strong<Resources::AtlasAnimation>>& animations,
			const std::vector<Strong<Texture2D>>& atlases);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

	protected:
		void Load_INTERNAL() override;
		void Map() override;

	private:
		AtlasAnimationTexture()
			: Texture3D("", {}) {}

		EPROPERTY()
		std::vector<Strong<AtlasAnimation>> m_animations_;

		EPROPERTY()
		std::vector<Strong<Texture2D>> m_atlases_;
	};
}
