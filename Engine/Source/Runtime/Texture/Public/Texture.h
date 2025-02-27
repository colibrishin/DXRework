#pragma once

#include "IGraphicAPI.h"

#include "Resource.h"
#include "ResourceManager.h"

#include "Texture.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_TEXTURE_API Texture : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		explicit Texture(std::filesystem::path path, eTexType type, const GenericTextureDescription& description);
		~Texture() override = default;

	public:
		Texture(const Texture& other);
		Texture& operator=(const Texture& other);
		
		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] eTexType GetPrimitiveTextureType() const;
		[[nodiscard]] const GenericTextureDescription& GetDescription() const;
		[[nodiscard]] ITexture* GetPrimitiveTexture() const;

		bool IsHotload() const;

	protected:
		// Derived class should hide these by their own case.
		virtual UINT64 GetWidth() const;
		virtual UINT   GetHeight() const;
		virtual UINT   GetDepth() const;

		virtual void Map() {};
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;
		
		void UpdateDescription(const GenericTextureDescription& description);
		
	private:
		friend struct Engine::ITexture;
		
		Texture();

		EPROPERTY()
		GenericTextureDescription m_desc_;
		EPROPERTY()
		eTexType m_type_;
		std::unique_ptr<ITexture> m_primitive_texture_;
	};
} // namespace Engine::Resources
