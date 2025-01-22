#pragma once
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine::Resources
{
	class Texture3D : public Texture
	{
	public:
		TEX_T(TEX_TYPE_3D)

		explicit Texture3D(const boost::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_3D, description) {}

		~Texture3D() override = default;

		UINT64 GetWidth() const final;
		UINT   GetHeight() const final;
		UINT   GetDepth() const final;

	protected:
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL

		Texture3D()
			: Texture("", TEX_TYPE_3D, {}) {}
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture3D)
