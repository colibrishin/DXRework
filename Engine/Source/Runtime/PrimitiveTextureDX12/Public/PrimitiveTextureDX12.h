#pragma once
#include <map>
#include <unordered_map>

#include <wrl/client.h>
#include <directx/d3d12.h>

#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine 
{
	struct PRIMITIVETEXTUREDX12_API DX12TextureMappingTask : public TextureMappingTask 
	{
		void Map(
			PrimitiveTexture* texture, 
			void* data_ptr, 
			const size_t width,
			const size_t height,
			const size_t stride,
			const size_t depth) override;
		void Map(
			PrimitiveTexture* lhs, 
			PrimitiveTexture* rhs,
			const UINT src_width,
			const UINT src_height,
			const size_t src_idx,
			const UINT dst_x,
			const UINT dst_y,
			const size_t dst_idx) override;

	private:
		uint64_t GetNextValue(const uint64_t key);
		void ReleaseUploadBuffer(const uint64_t ptr_id, const uint64_t bucket);

		using BucketKeyType = std::pair<uint64_t, uint64_t>;
		std::map<uint64_t, uint64_t> m_last_used_values_;
		std::unordered_map<BucketKeyType, ComPtr<ID3D12Resource>> m_upload_buffers_;
	};

	struct PRIMITIVETEXTUREDX12_API DX12PrimitiveTexture : public PrimitiveTexture
	{
		DX12PrimitiveTexture();
		void Generate(const Weak<Resources::Texture>& texture) override;
		void LoadFromFile(const Weak<Resources::Texture>& texture, const std::filesystem::path& path) override;
		void SaveAsFile(const std::filesystem::path& path) override;

	private:
		D3D12_RESOURCE_DIMENSION ConvertDimension(const eTexType type);
		void InitializeDescriptorHeaps();
		void InitializeResourceViews();

		GenericTextureDescription m_desc_;
		D3D12_RESOURCE_DESC m_native_desc_{};

		DXGI_FORMAT m_rtv_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT m_dsv_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
		
		ComPtr<ID3D12Resource> m_dx12_texture_;
		ComPtr<ID3D12Resource> m_upload_buffer_;

		ComPtr<ID3D12DescriptorHeap> m_srv_;
		ComPtr<ID3D12DescriptorHeap> m_dsv_;
		ComPtr<ID3D12DescriptorHeap> m_rtv_;
		ComPtr<ID3D12DescriptorHeap> m_uav_;
	};
}