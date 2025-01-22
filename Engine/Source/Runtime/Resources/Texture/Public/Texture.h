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
	enum TEXTURE_API eTexType
	{
		TEX_TYPE_1D,
		TEX_TYPE_2D,
		TEX_TYPE_3D
	};
}

namespace Engine::Resources
{
	class TEXTURE_API Texture : public Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_TEX)

#if defined(USE_DX12)
		struct TEXTURE_API GenericTextureDescription
		{
			D3D12_RESOURCE_DIMENSION Dimension        = static_cast<D3D12_RESOURCE_DIMENSION>(-1);
			UINT64                   Alignment        = 0;
			UINT64                   Width            = 0;
			UINT                     Height           = 0;
			UINT16                   DepthOrArraySize = 0;
			DXGI_FORMAT              Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
			D3D12_RESOURCE_FLAGS     Flags            = D3D12_RESOURCE_FLAG_NONE;
			UINT16                   MipsLevel        = 1;
			D3D12_TEXTURE_LAYOUT     Layout           = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE;
			DXGI_SAMPLE_DESC         SampleDesc       = {.Count = 1, .Quality = 0};
		};
#endif

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
		eTexType      GetPrimitiveTextureType() const;
		void LazyDescription(const GenericTextureDescription& desc);

#if defined(USE_DX12)
		ID3D12DescriptorHeap* GetSRVDescriptor() const;
		ID3D12DescriptorHeap* GetRTVDescriptor() const;
		ID3D12DescriptorHeap* GetDSVDescriptor() const;
		ID3D12DescriptorHeap* GetUAVDescriptor() const;
		ID3D12Resource*       GetRawResoruce() const;

		bool IsHotload() const;

		void Bind(
			const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap, eBindType type, UINT slot, UINT
			offset
		) const;
		void Bind(
			ID3D12GraphicsCommandList1* cmd, const DescriptorPtr& heap, eBindType type, UINT slot,
			UINT                        offset
		) const;
		void        Bind(const Weak<CommandPair>& cmd, const Texture& dsv) const;
		static void Bind(const Weak<CommandPair>& w_cmd, Texture** rtvs, UINT count, const Texture& dsv);

		void        Unbind(const Weak<CommandPair>& cmd, eBindType type) const;
		void        Unbind(ID3D12GraphicsCommandList1* cmd, eBindType type) const;
		void        Unbind(const Weak<CommandPair>& w_cmd, const Texture& dsv) const;
		static void Unbind(const Weak<CommandPair>& w_cmd, Texture** rtvs, UINT count, const Texture& dsv);

		void LazyRTV(const D3D12_RENDER_TARGET_VIEW_DESC& desc);
		void LazyDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
		void LazyUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
		void LazySRV(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

		void ManualTransition(
			ID3D12GraphicsCommandList1* cmd, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after
		) const;

		void Clear(ID3D12GraphicsCommandList1* cmd, D3D12_RESOURCE_STATES as) const;
#endif

		RESOURCE_SELF_INFER_GETTER_DECL(Texture)

	protected:
		// Derived class should hide these by their own case.
		virtual UINT64 GetWidth() const;
		virtual UINT   GetHeight() const;
		virtual UINT   GetDepth() const;
		virtual bool   DoesWantMapByResource() const;

#if defined(USE_DX12)
		const GenericTextureDescription& GetDescription() const;

		virtual void loadDerived(ComPtr<ID3D12Resource>& res) = 0;
		virtual bool map(char* mapped);
		virtual bool map(const Weak<CommandPair>& cmd, ID3D12Resource* texture_resource);

		[[nodiscard]] ID3D12Resource* GetRawResource() const;

		// Non-serialized
		ComPtr<ID3D12Resource> m_res_;

		ComPtr<ID3D12DescriptorHeap> m_srv_;
		ComPtr<ID3D12DescriptorHeap> m_rtv_;
		ComPtr<ID3D12DescriptorHeap> m_dsv_;
		ComPtr<ID3D12DescriptorHeap> m_uav_;
#endif

		void Unload_INTERNAL() override;

	private:
		Texture();
		void Load_INTERNAL() final;

		void InitializeDescriptorHeaps();

		void mapInternal();

		GenericTextureDescription m_desc_;
		eTexType                  m_type_;

#if defined(USE_DX12)
		bool                             m_custom_desc_[4];
		D3D12_RENDER_TARGET_VIEW_DESC    m_rtv_desc_{};
		D3D12_DEPTH_STENCIL_VIEW_DESC    m_dsv_desc_{};
		D3D12_UNORDERED_ACCESS_VIEW_DESC m_uav_desc_{};
		D3D12_SHADER_RESOURCE_VIEW_DESC  m_srv_desc_{};

		// Non-serialized
		ComPtr<ID3D12Resource> m_upload_buffer_;
#endif

		// Non-serialized
		bool                   m_b_lazy_window_;
	};
} // namespace Engine::Resources
