#pragma once
#if defined(USE_DX12)
#include <directx/d3d12.h>
#endif

#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"

// Static texture type, this should be added to every texture.
#define TEX_T(enum_val) static constexpr eTexType textype = enum_val;

namespace Engine
{
	struct PrimitiveTexture;
}

namespace Engine::Resources
{
	class TEXTURE_API Texture : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_TEX)
		explicit Texture(std::filesystem::path path, eTexType type, const GenericTextureDescription& description);
		~Texture() override;

	public:
		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		eResourceType GetResourceType() const override;
		eTexType GetPrimitiveTextureType() const;
		const GenericTextureDescription& GetDescription() const;
		bool IsHotload() const;

		RESOURCE_SELF_INFER_GETTER_DECL(Texture)

	protected:
		// Derived class should hide these by their own case.
		virtual UINT64 GetWidth() const;
		virtual UINT   GetHeight() const;
		virtual UINT   GetDepth() const;
		virtual bool   DoesWantMapByResource() const;
		void Unload_INTERNAL() override;

	private:
		friend struct Engine::PrimitiveTexture;
		
		Texture();
		void Load_INTERNAL() final;
		void UpdateDescription(const GenericTextureDescription& description);

		GenericTextureDescription m_desc_;
		std::unique_ptr<PrimitiveTexture> m_primitive_texture_;
		eTexType m_type_;
	};
} // namespace Engine::Resources
