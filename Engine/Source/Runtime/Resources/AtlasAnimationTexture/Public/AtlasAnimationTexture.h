#pragma once
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

namespace Engine::Resources
{
	class AtlasAnimationTexture;
}

POLYMORPHIC_TYPE_MAP(ENGINE_ATLASANIMATIONTEXTURE_API, Engine::Resources::AtlasAnimationTexture, Engine::Resources::Texture3D);

namespace Engine::Resources
{
	class ENGINE_ATLASANIMATIONTEXTURE_API AtlasAnimationTexture : public Texture3D
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(AtlasAnimationTexture)

		AtlasAnimationTexture(const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		
		RESOURCE_SELF_INFER_GETTER_DECL(AtlasAnimationTexture)

		static Strong<AtlasAnimationTexture> Create(
			const std::string& name, const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases
		);

	protected:
		void Load_INTERNAL() override;
		void Map() override;

	private:
		AtlasAnimationTexture()
			: Texture3D("", {}) {}

		std::vector<Strong<Texture2D>> m_atlases_;
	};
}
