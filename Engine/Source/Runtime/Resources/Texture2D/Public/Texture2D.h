#pragma once
#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/Resources/Texture/Public/Texture.h"

POLYMORPHIC_TYPE_MAP(Engine::Resources::Texture2D, Engine::Resources::Texture)

namespace Engine
{
	struct Texture2DModule;
}

POLYMORPHIC_TYPE_MAP(Engine::Texture2DModule, Engine::IModule);

namespace Engine
{
	struct Texture2DModule : public Engine::IModule
	{
		INLINE_COMPILE_TIME_TYPENAME(Texture2DModule)
		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

namespace Engine::Resources
{
	class ENGINE_TEXTURE2D_API Texture2D : public Texture
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Texture2D)

		explicit Texture2D(const std::filesystem::path& path, const GenericTextureDescription& description)
			: Texture(path, TEX_TYPE_2D, description) { }

		~Texture2D() override = default;

		RESOURCE_SELF_INFER_GETTER_DECL(Texture2D)

		static Strong<Texture2D> Create(
			const std::string&               name,
			const std::filesystem::path&     path,
			const GenericTextureDescription& desc
		);

		UINT64 GetWidth() const override;
		UINT   GetHeight() const override;
		UINT   GetDepth() const override;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL
		Texture2D()
			: Texture("", TEX_TYPE_2D, {}) {}

#if WITH_EDITOR
		friend struct Texture2DModule;
		static bool m_b_ui_load_dialog_;
#endif
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture2D)