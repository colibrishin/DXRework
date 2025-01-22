#pragma once

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderType.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"

POLYMORPHIC_TYPE_MAP(Engine::Resources::Texture, Engine::Abstracts::Resource)

namespace Engine::Resources
{
	class ENGINE_TEXTURE_API Texture : public Abstracts::Resource
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Texture)
		explicit Texture(std::filesystem::path path, eTexType type, const GenericTextureDescription& description);
		~Texture() override = default;

	public:
		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] eTexType GetPrimitiveTextureType() const;
		[[nodiscard]] const GenericTextureDescription& GetDescription() const;
		[[nodiscard]] PrimitiveTexture* GetPrimitiveTexture() const;

		bool IsHotload() const;
		RESOURCE_SELF_INFER_GETTER_DECL(Texture)

	protected:
		// Derived class should hide these by their own case.
		virtual UINT64 GetWidth() const;
		virtual UINT   GetHeight() const;
		virtual UINT   GetDepth() const;

		virtual void Map() {};
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend struct Engine::PrimitiveTexture;
		
		Texture();
		void UpdateDescription(const GenericTextureDescription& description);

		GenericTextureDescription m_desc_;
		std::unique_ptr<PrimitiveTexture> m_primitive_texture_;
		eTexType m_type_;
	};
} // namespace Engine::Resources
