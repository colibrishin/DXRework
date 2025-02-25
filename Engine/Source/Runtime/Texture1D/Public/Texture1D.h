#pragma once
#include "Texture.h"

#include "Texture1D.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_TEXTURE1D_API Texture1D : public Texture
	{
		GENERATE_BODY
	public:
		explicit Texture1D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_1D, description) { }

		~Texture1D() override = default;

		void OnDeserialized() override;

		UINT64 GetWidth() const final;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Texture1D()
			: Texture("", TEX_TYPE_1D, {}) {}

		UINT GetHeight() const final;
		UINT GetDepth() const final;
	};
} // namespace Engine::Resources
