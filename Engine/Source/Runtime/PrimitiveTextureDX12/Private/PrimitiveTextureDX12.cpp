#include "PrimitiveTextureDX12.h"

#include <directxtk12/ScreenGrab.h>

#include "ThrowIfFailed.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"

void Engine::DX12PrimitiveTexture::Generate(const Weak<Resources::Texture>& texture)
{
	if (const Strong<Resources::Texture>& tex = texture.lock()) 
	{
		m_desc_ = tex->GetDescription();

		if ((m_desc_.Flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) &&
			(m_desc_.Flags & RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
		{
			throw std::logic_error("Depth stencil and unordered cannot be flagged in same texture");
		}
		
		const auto& heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DIMENSION dim = ConvertDimension(m_desc_.Dimension);

		m_native_desc_ = 
		{
			.Dimension = dim,
			.Alignment = m_desc_.Alignment,
			.Width = m_desc_.Width,
			.Height = m_desc_.Height,
			.DepthOrArraySize = m_desc_.DepthOrArraySize,
			.MipLevels = m_desc_.MipsLevel,
			.Format = static_cast<DXGI_FORMAT>(m_desc_.Format),
			.SampleDesc = reinterpret_cast<const DXGI_SAMPLE_DESC&>(m_desc_.SampleDesc),
			.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(m_desc_.Layout),
			.Flags = static_cast<D3D12_RESOURCE_FLAGS>(m_desc_.Flags)
		};

		D3D12_CLEAR_VALUE clear_value = {};

		if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			clear_value.Format = static_cast<DXGI_FORMAT>(m_desc_.AsRTV != TEX_FORMAT_UNKNOWN ? m_desc_.AsRTV : m_desc_.Format);
			clear_value.Color[0] = 0.0f;
			clear_value.Color[1] = 0.0f;
			clear_value.Color[2] = 0.0f;
			clear_value.Color[3] = 1.0f;
		}

		if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			clear_value.Format = static_cast<DXGI_FORMAT>(m_desc_.AsDSV != TEX_FORMAT_UNKNOWN ? m_desc_.AsDSV : m_desc_.Format);
			clear_value.DepthStencil.Depth = 1.0f;
			clear_value.DepthStencil.Stencil = 0;
		}

		const D3D12_CLEAR_VALUE* cv_ptr = (
			m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
			(m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ?
			&clear_value :
			nullptr;

		DX::ThrowIfFailed
		(
			Managers::D3Device::GetInstance().GetDevice()->CreateCommittedResource
			(
				&heap_prop,
				D3D12_HEAP_FLAG_NONE,
				&m_native_desc_,
				D3D12_RESOURCE_STATE_COMMON,
				cv_ptr,
				IID_PPV_ARGS(m_dx12_texture_.GetAddressOf())
			)
		);

		if (const std::string& name = tex->GetName(); name.empty())
		{
			DX::ThrowIfFailed(m_dx12_texture_->SetName(L"Texture"));
		}
		else
		{
			const auto wname = L"Texture" + std::wstring(name.begin(), name.end());
			DX::ThrowIfFailed(m_dx12_texture_->SetName(wname.c_str()));
		}
		
		InitializeDescriptorHeaps();
		InitializeResourceViews();
	}
}

void Engine::DX12PrimitiveTexture::Map(void* src_ptr, const size_t stride, const size_t count)
{
	
}

void Engine::DX12PrimitiveTexture::LoadFromFile(const Weak<Resources::Texture>& texture, const std::filesystem::path& path)
{
	if (const Strong<Resources::Texture>& tex = texture.lock())
	{
		Engine::Managers::D3Device::GetInstance().CreateTextureFromFile
		(
			absolute(path),
			m_dx12_texture_.ReleaseAndGetAddressOf(),
			false
		);

		const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_COPY, L"Texture Uploading").lock();
		const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
		(
			m_dx12_texture_.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->SoftReset();
		cmd->GetList()->ResourceBarrier(1, &common_transition);
		cmd->FlagReady();

		const D3D12_RESOURCE_DESC desc = m_dx12_texture_->GetDesc();
		GenericTextureDescription tex_desc;

		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:
			tex_desc.Dimension = TEX_TYPE_UNKNOWN;
			break;
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			tex_desc.Dimension = TEX_TYPE_BUFFER;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			tex_desc.Dimension = TEX_TYPE_1D;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			tex_desc.Dimension = TEX_TYPE_2D;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			tex_desc.Dimension = TEX_TYPE_3D;
			break;
		};

		tex_desc.Format = static_cast<eFormat>(desc.Format);
		tex_desc.Alignment = desc.Alignment;
		tex_desc.Width = desc.Width;
		tex_desc.Height = desc.Height;
		tex_desc.MipsLevel = desc.MipLevels;
		tex_desc.SampleDesc = reinterpret_cast<const SamplerDescription&>(desc.SampleDesc);
		tex_desc.Layout = static_cast<eTextureLayout>(desc.Layout);
		tex_desc.Flags = desc.Flags;
		tex_desc.DepthOrArraySize = desc.DepthOrArraySize;

		UpdateDescription(tex, tex_desc);
	}
}

void Engine::DX12PrimitiveTexture::InitializeDescriptorHeaps()
{
	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
		(
			Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
			(
				&desc,
				IID_PPV_ARGS(m_srv_.GetAddressOf())
			));
	}
	
	if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
		(
			Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
			(
				&desc,
				IID_PPV_ARGS(m_uav_.GetAddressOf())
			));
	}

	if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
		(
			Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
			(
				&desc,
				IID_PPV_ARGS(m_dsv_.GetAddressOf())
			));
	}

	if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		
		DX::ThrowIfFailed
		(
			Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
			(
				&desc,
				IID_PPV_ARGS(m_rtv_.GetAddressOf())
			));
	}
}

void Engine::DX12PrimitiveTexture::InitializeResourceViews()
{
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		if (m_desc_.AsSRV)
		{
			desc = reinterpret_cast<const D3D12_SHADER_RESOURCE_VIEW_DESC&>(m_desc_.Srv);
		}
	
		Managers::D3Device::GetInstance().GetDevice()->CreateShaderResourceView
		(
			m_dx12_texture_.Get(),
			&desc,
			m_srv_->GetCPUDescriptorHandleForHeapStart()
		);
	}

	if (m_desc_.AsRTV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc = reinterpret_cast<const D3D12_RENDER_TARGET_VIEW_DESC&>(m_desc_.Rtv);
		
		Managers::D3Device::GetInstance().GetDevice()->CreateRenderTargetView
		(
			m_dx12_texture_.Get(),
			&desc,
			m_srv_->GetCPUDescriptorHandleForHeapStart());
	}

	if (m_desc_.AsDSV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
		desc = reinterpret_cast<const D3D12_DEPTH_STENCIL_VIEW_DESC&>(m_desc_.Dsv);
		
		Managers::D3Device::GetInstance().GetDevice()->CreateDepthStencilView
		(
			m_dx12_texture_.Get(),
			&desc,
			m_dsv_->GetCPUDescriptorHandleForHeapStart());
	}
	
	if (m_desc_.AsUAV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		desc = reinterpret_cast<const D3D12_UNORDERED_ACCESS_VIEW_DESC&>(m_desc_.Uav);
		
		Managers::D3Device::GetInstance().GetDevice()->CreateUnorderedAccessView
		(
			m_dx12_texture_.Get(),
			nullptr,
			nullptr,
			m_uav_->GetCPUDescriptorHandleForHeapStart());
	}
}

void Engine::DX12PrimitiveTexture::SaveAsFile(const std::filesystem::path& path)
{
	DX::ThrowIfFailed
	(
		DirectX::SaveDDSTextureToFile
		(
			Managers::D3Device::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
			m_dx12_texture_.Get(),
			path.c_str(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COMMON
		)
	);
}

D3D12_RESOURCE_DIMENSION Engine::DX12PrimitiveTexture::ConvertDimension(const eTexType type)
{
	switch (type)
	{
	case TEX_TYPE_UNKNOWN:
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	case TEX_TYPE_1D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case TEX_TYPE_2D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case TEX_TYPE_3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	case TEX_TYPE_BUFFER:
		return D3D12_RESOURCE_DIMENSION_BUFFER;
	}
	
	return D3D12_RESOURCE_DIMENSION_UNKNOWN;
}
