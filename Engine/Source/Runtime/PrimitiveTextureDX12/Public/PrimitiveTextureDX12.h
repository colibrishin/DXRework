#pragma once
#include <wrl/client.h>
#include <directx/d3d12.h>

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine 
{
	struct PRIMITIVETEXTUREDX12_API DX12PrimitiveTexture : public PrimitiveTexture
	{
		void Generate(const Weak<Resources::Texture>& texture) override;
		void Map(void* src_ptr, const size_t stride, const size_t count) override;
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
		
		ComPtr<ID3D12DescriptorHeap> m_srv_;
		ComPtr<ID3D12DescriptorHeap> m_dsv_;
		ComPtr<ID3D12DescriptorHeap> m_rtv_;
		ComPtr<ID3D12DescriptorHeap> m_uav_;
	};
}