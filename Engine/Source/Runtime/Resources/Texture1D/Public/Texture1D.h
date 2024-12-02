#pragma once
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine::Resources
{
	class Texture1D : public Texture
	{
	public:
		TEX_T(TEX_TYPE_1D)

		explicit Texture1D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_1D, description) { }

		~Texture1D() override = default;

		void OnDeserialized() override;

		UINT64 GetWidth() const final;

	protected:
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL

		Texture1D()
			: Texture("", TEX_TYPE_1D, {}) {}

		UINT GetHeight() const final;
		UINT GetDepth() const final;
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture1D)
