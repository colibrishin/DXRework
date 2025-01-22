#include "D3D12PrimitiveTexture.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <DirectXTex.h>

#include <directxtk12/ScreenGrab.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/DDSTextureLoader.h>

#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"

#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "Source/Runtime/D3D12GraphicInterface/Public/ThrowIfFailed.h"

#include "source/runtime/D3D12GraphicInterface/Public/CommandPair.h"
#include "source/runtime/D3D12GraphicInterface/Public/D3D12GraphicInterface.h"

Engine::D3D12PrimitiveTexture::D3D12PrimitiveTexture() {}

void Engine::D3D12PrimitiveTexture::Generate(Engine::Resources::Texture* texture)
{
	m_description_ = texture->GetDescription();

	if ((m_description_.Flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) &&
		(m_description_.Flags & RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
	{
		throw std::logic_error("Depth stencil and unordered cannot be flagged in same texture");
	}
	
	const auto& heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_DIMENSION dim = ConvertDimension(m_description_.Dimension);

	m_native_desc_ = 
	{
		.Dimension = dim,
		.Alignment = m_description_.Alignment,
		.Width = m_description_.Width,
		.Height = m_description_.Height,
		.DepthOrArraySize = m_description_.DepthOrArraySize,
		.MipLevels = m_description_.MipsLevel,
		.Format = static_cast<DXGI_FORMAT>(m_description_.Format),
		.SampleDesc = reinterpret_cast<const DXGI_SAMPLE_DESC&>(m_description_.SampleDesc),
		.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(m_description_.Layout),
		.Flags = static_cast<D3D12_RESOURCE_FLAGS>(m_description_.Flags)
	};

	D3D12_CLEAR_VALUE clear_value = {};

	if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		if (m_description_.AsRTV)
		{
			if (m_description_.Rtv.Format == DXGI_FORMAT_UNKNOWN)
			{
				clear_value.Format = m_native_desc_.Format;
			}
			else
			{
				clear_value.Format = static_cast<DXGI_FORMAT>(m_description_.Rtv.Format);
			}
		}

		clear_value.Color[0] = 0.0f;
		clear_value.Color[1] = 0.0f;
		clear_value.Color[2] = 0.0f;
		clear_value.Color[3] = 1.0f;
	}

	if (m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		if (m_description_.AsDSV)
		{
			if (m_description_.Dsv.Format == DXGI_FORMAT_UNKNOWN)
			{
				clear_value.Format = m_native_desc_.Format;
			}
			else
			{
				clear_value.Format = static_cast<DXGI_FORMAT>(m_description_.Dsv.Format);
			}
		}
		clear_value.DepthStencil.Depth = 1.0f;
		clear_value.DepthStencil.Stencil = 0;
	}

	const D3D12_CLEAR_VALUE* cv_ptr = (
		m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
		(m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ?
		&clear_value :
		nullptr;

	const auto dev = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());

	DX::ThrowIfFailed
	(
		dev->CreateCommittedResource
		(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&m_native_desc_,
			D3D12_RESOURCE_STATE_COMMON,
			cv_ptr,
			IID_PPV_ARGS(m_dx12_texture_.GetAddressOf())
		)
	);

	if (const std::string& name = texture->GetName(); name.empty())
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

	SetPrimitiveTexture(m_dx12_texture_.Get());
}

void Engine::D3D12PrimitiveTexture::LoadFromFile(Engine::Resources::Texture* texture, const std::filesystem::path& path)
{
	const auto dev    = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());
	auto&      native = reinterpret_cast<D3D12GraphicInterface&>(GraphicInterfaceAccessor::GetInterface());

	ID3D12CommandQueue* queue = native.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	if (!exists(path))
	{
		throw std::runtime_error("File not found.");
	}

	DirectX::ResourceUploadBatch resource_upload_batch(dev);

	resource_upload_batch.Begin();

	if (path.extension() == ".dds")
	{
		DX::ThrowIfFailed
				(
				 CreateDDSTextureFromFile
				 (
				  dev,
				  resource_upload_batch,
				  path.c_str(),
				  m_dx12_texture_.GetAddressOf(),
				  false
				 )
				);
	}
	else
	{
		DX::ThrowIfFailed
				(
				 CreateWICTextureFromFile
				 (
				  dev,
				  resource_upload_batch,
				  path.c_str(),
				  m_dx12_texture_.GetAddressOf(),
				  false
				 )
				);
	}

	const auto& token = resource_upload_batch.End(queue);
	token.wait();

	const GraphicInterfaceContextReturnType& context = GraphicInterfaceAccessor::GetInterface().GetNewContext(0, false, L"Texture Uploading");
	const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

	const auto& cmd = reinterpret_cast<CommandPair*>(primitive.commandList);
	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_dx12_texture_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->SoftReset();
	cmd->GetList()->ResourceBarrier(1, &common_transition);
	cmd->FlagReady();

	SetPrimitiveTexture(m_dx12_texture_.Get());
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

	InitializeDescriptorHeaps();
	InitializeResourceViews();
	
	UpdateDescription(tex_desc);
}

void Engine::D3D12PrimitiveTexture::InitializeDescriptorHeaps()
{
	const auto dev = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());

	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
		(
			dev->CreateDescriptorHeap
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
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		DX::ThrowIfFailed
		(
			dev->CreateDescriptorHeap
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
			dev->CreateDescriptorHeap
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
			dev->CreateDescriptorHeap
			(
				&desc,
				IID_PPV_ARGS(m_rtv_.GetAddressOf())
			));
	}
}

void Engine::D3D12PrimitiveTexture::InitializeResourceViews() const
{
	const auto& compare = [](const void* a, const void* b, size_t length)
	{
		auto ba = static_cast<const char*>(a);
		auto bb = static_cast<const char*>(b);
		while (length--)
		{
			if (*ba != *bb)
			{
				return false;
			}

			++ba;
			++bb;
		}

		return true;
	};

	const auto dev = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());
	
	if (m_description_.AsSRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_SHADER_RESOURCE_VIEW_DESC* target = &desc;

		//todo: due to the alignment padding, the struct cannot directly compared.
		desc = reinterpret_cast<const D3D12_SHADER_RESOURCE_VIEW_DESC&>(m_description_.Srv);
		static constexpr D3D12_SHADER_RESOURCE_VIEW_DESC empty_desc{};
		if (compare(&desc, &empty_desc, sizeof(decltype(desc))))
		{
			target = nullptr;
		}

		dev->CreateShaderResourceView
		(
			m_dx12_texture_.Get(),
			target,
			m_srv_->GetCPUDescriptorHandleForHeapStart()
		);
	}

	if (m_description_.AsRTV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		D3D12_RENDER_TARGET_VIEW_DESC* target = &desc;

		//todo: due to the alignment padding, the struct cannot directly compared.
		desc = reinterpret_cast<const D3D12_RENDER_TARGET_VIEW_DESC&>(m_description_.Rtv);
		static constexpr D3D12_RENDER_TARGET_VIEW_DESC empty_desc{};
		if (compare(&desc, &empty_desc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC)))
		{
			target = nullptr;
		}

		dev->CreateRenderTargetView
		(
			m_dx12_texture_.Get(),
			target,
			m_rtv_->GetCPUDescriptorHandleForHeapStart());
	}

	if (m_description_.AsDSV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
		D3D12_DEPTH_STENCIL_VIEW_DESC* target = &desc;

		//todo: due to the alignment padding, the struct cannot directly compared.
		desc = reinterpret_cast<const D3D12_DEPTH_STENCIL_VIEW_DESC&>(m_description_.Dsv);
		static constexpr D3D12_DEPTH_STENCIL_VIEW_DESC empty_desc{};
		if (compare(&desc, &empty_desc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC)))
		{
			target = nullptr;
		}

		dev->CreateDepthStencilView
		(
			m_dx12_texture_.Get(),
			target,
			m_dsv_->GetCPUDescriptorHandleForHeapStart());
	}
	
	if (m_description_.AsUAV && m_native_desc_.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		D3D12_UNORDERED_ACCESS_VIEW_DESC* target = &desc;

		//todo: due to the alignment padding, the struct cannot directly compared.
		desc = reinterpret_cast<const D3D12_UNORDERED_ACCESS_VIEW_DESC&>(m_description_.Uav);
		static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC empty_desc{};
		if (compare(&desc, &empty_desc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC)))
		{
			target = nullptr;
		}

		dev->CreateUnorderedAccessView
		(
			m_dx12_texture_.Get(),
			nullptr,
			target,
			m_uav_->GetCPUDescriptorHandleForHeapStart());
	}
}

void Engine::D3D12PrimitiveTexture::SaveAsFile(const std::filesystem::path& path)
{
	auto& native = reinterpret_cast<D3D12GraphicInterface&>(GraphicInterfaceAccessor::GetInterface());
	ID3D12CommandQueue* queue = native.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	DX::ThrowIfFailed
	(
		DirectX::SaveDDSTextureToFile
		(
			queue,
			m_dx12_texture_.Get(),
			path.c_str(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COMMON
		)
	);
}

void Engine::D3D12PrimitiveTexture::Map(
	void* data_ptr, const size_t width, const size_t height, const size_t stride, const size_t depth
)
{
	const auto dev = static_cast<ID3D12Device2*>(GraphicInterfaceAccessor::GetInterface().GetNativeInterface());
	const GenericTextureDescription& desc = GetDescription();

	const size_t pixel_in_bytes = DirectX::BitsPerPixel(static_cast<DXGI_FORMAT>(desc.Format)) / 8;

	// Align(Width * format bytes, 256) = Row pitch
	const size_t row_pitch = Align
	(
		desc.Width * pixel_in_bytes,
		D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
	);

	// RowPitch * Height = Slice pitch
	const size_t slice_pitch = Align(desc.Height * row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	size_t total_bytes = row_pitch;

	if (slice_pitch > 0) 
	{
		total_bytes = row_pitch * slice_pitch;
	}

	DX::ThrowIfFailed
	(
		DirectX::CreateUploadBuffer
		(
			dev,
			nullptr,
			total_bytes,
			pixel_in_bytes,
			m_upload_buffer_.GetAddressOf()
		)
	);

	char* mapped = nullptr;
	char* byte_ptr = static_cast<char*>(data_ptr);

	DX::ThrowIfFailed
	(
		m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&mapped))
	);
	
	for (UINT64 i = 0; i < desc.DepthOrArraySize; ++i)
	{
		const UINT64 d = slice_pitch * i;

		for (UINT64 j = 0; j < desc.Height; ++j)
		{
			if (j >= height) 
			{
				break;
			}

			const UINT64 h = row_pitch * j;

			for (UINT64 k = 0; k < desc.Width; ++k)
			{
				if (k >= width) 
				{
					break;
				}

				SIMDExtension::_mm256_memcpy(
					mapped + d + h + k * stride,
					byte_ptr + i + j + k * stride,
					stride);
			}
		}
	}
	
	m_upload_buffer_->Unmap(0, nullptr);

	const auto                               texture_res = static_cast<ID3D12Resource*>(GetNativeTexture());
	const GraphicInterfaceContextReturnType& context     = GraphicInterfaceAccessor::GetInterface().GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Texture mapping");
	const GraphicInterfaceContextPrimitive&  primitive   = context.GetPointers();
	auto                                     cmd         = static_cast<CommandPair*>(primitive.commandList);

	const auto dst = CD3DX12_TEXTURE_COPY_LOCATION(texture_res, 0);
	auto       src = CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer_.Get(), 0);
	
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(row_pitch);
	src.PlacedFootprint.Footprint.Depth = desc.DepthOrArraySize;
	src.PlacedFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
	src.PlacedFootprint.Footprint.Height = desc.Height;
	src.PlacedFootprint.Footprint.Format = static_cast<DXGI_FORMAT>(desc.Format);

	const auto& dest_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		texture_res,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	const auto& dst_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
	(
		texture_res,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->SoftReset();
	cmd->GetList()->ResourceBarrier(1, &dest_transition);
	cmd->GetList()->CopyTextureRegion
	(
		&dst,
		0, 0, 0,
		&src,
		nullptr
	);

	cmd->GetList()->ResourceBarrier(1, &dst_transition_back);
	cmd->Execute();
}

void Engine::D3D12PrimitiveTexture::Map(
	PrimitiveTexture* src, const UINT src_width, const UINT src_height, const size_t src_idx, const UINT dst_x,
	const UINT dst_y, const size_t dst_idx
)
{
	const GraphicInterfaceContextReturnType& context = GraphicInterfaceAccessor::GetInterface().GetNewContext(D3D12_COMMAND_LIST_TYPE_COPY, false, L"Texture Copy");
	const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
	CommandPair* cmd = static_cast<CommandPair*>(primitive.commandList);
	
	D3D12PrimitiveTexture* native_src = reinterpret_cast<D3D12PrimitiveTexture*>(src);

	ID3D12Resource* src_res = static_cast<ID3D12Resource*>(native_src->GetNativeTexture());
	ID3D12Resource* dst_res = static_cast<ID3D12Resource*>(GetNativeTexture());

	D3D12_BOX box = 
	{
		0, 0, 0, 
		src_width, src_height, static_cast<UINT>(src_idx)
	};

	D3D12_TEXTURE_COPY_LOCATION dst_desc
	{
		.pResource = dst_res,
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = 0
	};

	D3D12_TEXTURE_COPY_LOCATION src_desc
	{
		.pResource = src_res,
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = 0
	};

	cmd->SoftReset();
	cmd->GetList()->CopyTextureRegion
	(
		&dst_desc,
		dst_x,
		dst_y,
		dst_idx,
		&src_desc,
		&box
	);
	cmd->Execute();
}

ID3D12DescriptorHeap* Engine::D3D12PrimitiveTexture::GetSrv() const
{
	return m_srv_.Get();
}

ID3D12DescriptorHeap* Engine::D3D12PrimitiveTexture::GetDsv() const
{
	return m_dsv_.Get();
}

ID3D12DescriptorHeap* Engine::D3D12PrimitiveTexture::GetRtv() const
{
	return m_rtv_.Get();
}

ID3D12DescriptorHeap* Engine::D3D12PrimitiveTexture::GetUav() const
{
	return m_uav_.Get();
}

D3D12_RESOURCE_DIMENSION Engine::D3D12PrimitiveTexture::ConvertDimension(const eTexType type)
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
