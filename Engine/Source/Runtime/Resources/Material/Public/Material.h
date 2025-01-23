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
		typedef std::array<Strong<Texture>, g_max_texture_per_material> StrongTextureArray;
		typedef std::array<Weak<Texture>, g_max_texture_per_material> WeakTextureArray;

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
		void SwapTexture(const size_t before, const size_t after);
		void SetAtlasTexture(const Weak<AtlasAnimationTexture>& texture);
		void SetShader(const Weak<Shader>& shader);

		[[nodiscard]] const Graphics::MaterialPrimitive& GetPrimitive() const;
		[[nodiscard]] const WeakTextureArray& GetTextures() const;
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
		MaterialPrimitive m_material_sb_;

		EPROPERTY()
		MetadataPath m_shader_path_;

		EPROPERTY()
		std::array<MetadataPath, g_max_texture_per_material> m_texture_paths_;

		EPROPERTY()
		MetadataPath m_atlas_path_;

#if WITH_EDITOR
		bool m_ui_shader_dialog_ = false;
		bool m_ui_add_dialog_ = false;
#endif

		Strong<Shader> m_shader_;
		StrongTextureArray m_textures_;
		Strong<AtlasAnimationTexture> m_atlas_;

		Weak<Shader> m_cached_shader_;
		WeakTextureArray m_cached_textures_;
		Weak<AtlasAnimationTexture> m_cached_atlas_;
	};
}