#pragma once
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine::Resources
{
	class TEXTURE2D_API Texture2D : public Texture
	{
	public:
		TEX_T(TEX_TYPE_2D)

		explicit Texture2D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_2D, description) { }

		~Texture2D() override = default;

		RESOURCE_SELF_INFER_GETTER_DECL(Texture2D)

		static boost::shared_ptr<Texture2D> Create(
			const std::string&               name,
			const std::filesystem::path&     path,
			const GenericTextureDescription& desc
		);

		UINT64 GetWidth() const override;
		UINT   GetHeight() const override;
		UINT   GetDepth() const override;

	protected:
		void loadDerived(ComPtr<ID3D12Resource>& res) override;
		void Unload_INTERNAL() override;

	private:
		Texture2D()
			: Texture("", TEX_TYPE_2D, {}) {}
	};
} // namespace Engine::Resources
