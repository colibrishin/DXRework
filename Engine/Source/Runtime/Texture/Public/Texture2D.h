#pragma once
#include "Texture.h"

#include "Texture2D.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_TEXTURE_API Texture2D : public Texture
	{
		GENERATE_BODY
	public:
		explicit Texture2D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_2D, description) { }

		~Texture2D() override = default;

		UINT64 GetWidth() const override;
		UINT   GetHeight() const override;
		UINT   GetDepth() const override;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Texture2D()
			: Texture("", TEX_TYPE_2D, {}) {}

#if WITH_EDITOR
		friend struct TextureModule;
		static bool m_b_ui_load_dialog_;
#endif
	};
} // namespace Engine::Resources
