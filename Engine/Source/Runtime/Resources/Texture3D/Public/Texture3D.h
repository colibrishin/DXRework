#pragma once
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine::Resources
{
	class TEXTURE3D_API Texture3D : public Texture
	{
	public:
		TEX_T(TEX_TYPE_3D)

		explicit Texture3D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_3D, description) {}

		~Texture3D() override = default;

		UINT64 GetWidth() const final;
		UINT   GetHeight() const final;
		UINT   GetDepth() const final;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:

		Texture3D()
			: Texture("", TEX_TYPE_3D, {}) {}
	};
} // namespace Engine::Resources
