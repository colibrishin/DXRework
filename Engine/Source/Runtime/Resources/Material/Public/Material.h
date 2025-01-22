#pragma once
#include "Source/Runtime/Core/StructuredBuffer/Public/StructuredBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "AtlasAnimationTexture.h"
#include "AtlasAnimation.h"
#include "MaterialPrimitive.h"

#include "Material.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_MATERIAL_API Material final : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		typedef std::array<Strong<Texture>, BIND_SLOT_END> TextureArray;

		Material(const Graphics::MaterialPrimitive& material);

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void SetTexture(const Weak<Texture>& texture, const size_t slot = 0);
		void SetAtlasTexture(const Weak<AtlasAnimationTexture>& texture);
		void SetShader(const Weak<Shader>& shader);

		[[nodiscard]] const Graphics::MaterialPrimitive& GetMaterialPrimitive() const;
		[[nodiscard]] const TextureArray& GetTextures() const;
		[[nodiscard]] Weak<AtlasAnimationTexture> GetAtlasTexture() const;
		[[nodiscard]] Weak<AtlasAnimation>        GetAtlasAnimation(const size_t idx) const;
		[[nodiscard]] Weak<Shader>                GetShader() const;

	private:
		Material();

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		EPROPERTY()
		Graphics::MaterialPrimitive m_material_sb_;

		EPROPERTY()
		MetadataPath m_shader_path_;

		EPROPERTY()
		std::array<MetadataPath, BIND_SLOT_END> m_texture_paths_;

		EPROPERTY()
		MetadataPath m_atlas_path_;

#if WITH_EDITOR
		bool m_ui_add_dialog_ = false;
#endif

		Strong<Shader> m_shader_;

		TextureArray m_textures_;

		Strong<AtlasAnimationTexture> m_atlas_loaded_;
	};
}