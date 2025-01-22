#include "PrimitiveTextureDX12.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <DirectXTex.h>

#include <directxtk12/ScreenGrab.h>
#include <directxtk12/BufferHelpers.h>

#include "ThrowIfFailed.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"
#include "Source/Runtime/RenderPassTaskDX12/Public/RenderPassTaskDX12.h"

Engine::DX12PrimitiveTexture::DX12PrimitiveTexture()
{
	SetTextureMappingTask<DX12TextureMappingTask>();
	SetTextureBindingTask<DX12TextureBindingTask>();
}

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

		SetPrimitiveTexture(m_dx12_texture_.Get());
	}
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

ID3D12DescriptorHeap* Engine::DX12PrimitiveTexture::GetSrv() const
{
	return m_srv_.Get();
}

ID3D12DescriptorHeap* Engine::DX12PrimitiveTexture::GetDsv() const
{
	return m_dsv_.Get();
}

ID3D12DescriptorHeap* Engine::DX12PrimitiveTexture::GetRtv() const
{
	return m_rtv_.Get();
}

ID3D12DescriptorHeap* Engine::DX12PrimitiveTexture::GetUav() const
{
	return m_uav_.Get();
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

void Engine::DX12TextureMappingTask::Map(
	PrimitiveTexture* texture, 
	void* data_ptr,
	const size_t width,
	const size_t height,
	const size_t stride,
	const size_t depth)
{
	const GenericTextureDescription& desc = texture->GetDescription();

	size_t pixel_in_bytes = DirectX::BitsPerPixel(static_cast<DXGI_FORMAT>(desc.Format)) / 8;

	// Align(Width * format bytes, 256) = Row pitch
	size_t row_pitch = Align
	(
		desc.Width * pixel_in_bytes,
		D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
	);

	// RowPitch * Height = Slice pitch
	size_t slice_pitch = Align(desc.Height * row_pitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	size_t total_bytes = row_pitch;

	if (slice_pitch > 0) 
	{
		total_bytes = row_pitch * slice_pitch;
	}

	const uint64_t ptr_id = reinterpret_cast<uint64_t>(texture);
	const uint64_t new_id = GetNextValue(ptr_id);
	ComPtr<ID3D12Resource>& upload_res = m_upload_buffers_[{ptr_id, new_id}];

	DX::ThrowIfFailed
	(
		DirectX::CreateUploadBuffer
		(
			Managers::D3Device::GetInstance().GetDevice(),
			nullptr,
			total_bytes,
			pixel_in_bytes,
			upload_res.GetAddressOf()
		)
	);

	char* mapped = nullptr;
	char* byte_ptr = static_cast<char*>(data_ptr);

	DX::ThrowIfFailed
	(
		upload_res->Map(0, nullptr, reinterpret_cast<void**>(&mapped))
	);

	const GenericTextureDescription& desc = texture->GetDescription();
	
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
	
	upload_res->Unmap(0, nullptr);

	ID3D12Resource* texture_res = reinterpret_cast<ID3D12Resource*>(texture->GetPrimitiveTexture());

	const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_COPY, L"Texture Mapping").lock();

	const auto dst = CD3DX12_TEXTURE_COPY_LOCATION(texture_res, 0);
	auto       src = CD3DX12_TEXTURE_COPY_LOCATION(upload_res.Get(), 0);
	
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
	cmd->FlagReady(
		[this, ptr_id, new_id]() 
		{
			ReleaseUploadBuffer(ptr_id, new_id);
		});
}

uint64_t Engine::DX12TextureMappingTask::GetNextValue(const uint64_t key)
{
	if (!m_last_used_values_.contains(key)) 
	{
		return 0;
	}
	
	return m_last_used_values_.at(key) + 1;
}

void Engine::DX12TextureMappingTask::ReleaseUploadBuffer(const uint64_t ptr_id, const uint64_t bucket)
{
	if (m_upload_buffers_.contains({ ptr_id, bucket }))
	{
		m_upload_buffers_.at({ ptr_id, bucket })->Release();
		m_upload_buffers_.erase({ptr_id, bucket});
	}
}

void Engine::DX12TextureMappingTask::Map(
	PrimitiveTexture* src, 
	PrimitiveTexture* dst,
	const UINT src_width,
	const UINT src_height,
	const size_t src_idx,
	const UINT dst_x,
	const UINT dst_y,
	const size_t dst_idx)
{
	const Strong<CommandPair>& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_COPY, L"Texture Copy").lock();

	DX12PrimitiveTexture* native_src = reinterpret_cast<DX12PrimitiveTexture*>(src);
	DX12PrimitiveTexture* native_dst = reinterpret_cast<DX12PrimitiveTexture*>(dst);

	ID3D12Resource* src_res = reinterpret_cast<ID3D12Resource*>(native_src->GetPrimitiveTexture());
	ID3D12Resource* dst_res = reinterpret_cast<ID3D12Resource*>(native_dst->GetPrimitiveTexture());

	D3D12_BOX box = 
	{
		0, 0, 0, 
		src_width, src_height, src_idx
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

	cmd->GetList()->CopyTextureRegion
	(
		&dst_desc,
		dst_x,
		dst_y,
		dst_idx,
		&src_desc,
		&box
	);
}

void Engine::DX12TextureBindingTask::Bind(RenderPassTask* task_context, PrimitiveTexture* texture, const eBindType bind_type, const UINT bind_slot, const UINT offset)
{
	DX12RenderPassTask* task = reinterpret_cast<DX12RenderPassTask*>(task_context);
	DX12PrimitiveTexture* tex = reinterpret_cast<DX12PrimitiveTexture*>(texture);
	
	ID3D12Resource* res = reinterpret_cast<ID3D12Resource*>(tex->GetPrimitiveTexture());
	CommandPair* cmd = task->GetCurrentCommandList();
	DescriptorPtrImpl* heap = task->GetCurrentHeap();

	switch (bind_type)
	{
	case BIND_TYPE_SRV:
	{
		const auto& srv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
		);

		cmd->GetList()->ResourceBarrier(1, &srv_trans);
		heap->SetShaderResource(tex->GetSrv()->GetCPUDescriptorHandleForHeapStart(), bind_slot + offset);
		break;
	}
	case BIND_TYPE_UAV:
	{
		const auto& uav_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

		cmd->GetList()->ResourceBarrier(1, &uav_trans);
		heap->SetUnorderedAccess(tex->GetUav()->GetCPUDescriptorHandleForHeapStart(), bind_slot + offset);
		break;
	}
	case BIND_TYPE_RTV:
	{
		const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		cmd->GetList()->ResourceBarrier(1, &rtv_trans);

		const D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle[]
		{
			tex->GetRtv()->GetCPUDescriptorHandleForHeapStart()
		};

		cmd->GetList()->OMSetRenderTargets
		(
			1,
			rtv_handle,
			false,
			nullptr
		);
		break;
	}
	case BIND_TYPE_DSV:
	{
		break;
	}
	case BIND_TYPE_DSV_ONLY:
	{
		const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

		cmd->GetList()->ResourceBarrier(1, &dsv_trans);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle[]
		{
			tex->GetDsv()->GetCPUDescriptorHandleForHeapStart()
		};

		cmd->GetList()->OMSetRenderTargets
		(
			0,
			nullptr,
			false,
			dsv_handle
		);

		break;
	}
	case BIND_TYPE_SAMPLER:
	case BIND_TYPE_CB:
	case BIND_TYPE_COUNT:
	default:
		break;
	}
}

void Engine::DX12TextureBindingTask::Unbind(RenderPassTask* task_context, PrimitiveTexture* texture, const eBindType previous_bind_type)
{
	DX12RenderPassTask* task = reinterpret_cast<DX12RenderPassTask*>(task_context);
	DX12PrimitiveTexture* tex = reinterpret_cast<DX12PrimitiveTexture*>(texture);

	ID3D12Resource* res = reinterpret_cast<ID3D12Resource*>(tex->GetPrimitiveTexture());
	CommandPair* cmd = task->GetCurrentCommandList();
	DescriptorPtrImpl* heap = task->GetCurrentHeap();

	switch (previous_bind_type)
	{
	case BIND_TYPE_UAV:
		const auto& uav_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->GetList()->ResourceBarrier(1, &uav_trans);
		break;
	case BIND_TYPE_SRV:
		const auto& srv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->GetList()->ResourceBarrier(1, &srv_trans);
		break;
	case BIND_TYPE_RTV:
		const auto& rtv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->GetList()->ResourceBarrier(1, &rtv_trans);
		break;
	case BIND_TYPE_DSV:
	case BIND_TYPE_DSV_ONLY:
		const auto& dsv_trans = CD3DX12_RESOURCE_BARRIER::Transition
		(
			res,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->GetList()->ResourceBarrier(1, &dsv_trans);
		break;
	case BIND_TYPE_SAMPLER:
		break;
	case BIND_TYPE_CB:
		break;
	case BIND_TYPE_COUNT:
		break;
	default:;
	}
}

void Engine::DX12TextureBindingTask::BindMultiple(RenderPassTask* task_context, PrimitiveTexture* const* rtvs, const size_t rtv_count, PrimitiveTexture* dsv)
{
	DX12RenderPassTask* task = reinterpret_cast<DX12RenderPassTask*>(task_context);

	CommandPair* cmd = task->GetCurrentCommandList();
	DescriptorPtrImpl* heap = task->GetCurrentHeap();

	std::vector<D3D12_RESOURCE_BARRIER> transitions;
	transitions.reserve(rtv_count + 1);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs_heap;
	rtvs_heap.reserve(rtv_count);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv_heap;

	for (size_t i = 0; i < rtv_count; ++i) 
	{
		DX12PrimitiveTexture* rtv = reinterpret_cast<DX12PrimitiveTexture*>(rtvs[i]);
		
		const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
		(
			static_cast<ID3D12Resource*>(rtvs[i]->GetPrimitiveTexture()),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		rtvs_heap.push_back(static_cast<DX12PrimitiveTexture*>(rtvs[i])->GetRtv()->GetCPUDescriptorHandleForHeapStart());
		transitions.push_back(rtv_transition);
	}

	DX12PrimitiveTexture* native_dsv = reinterpret_cast<DX12PrimitiveTexture*>(dsv);
	dsv_heap = native_dsv->GetDsv()->GetCPUDescriptorHandleForHeapStart();

	const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		static_cast<ID3D12Resource*>(native_dsv->GetPrimitiveTexture()),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	transitions.push_back(dsv_transition);

	cmd->GetList()->ResourceBarrier(static_cast<UINT>(transitions.size()), transitions.data());
	cmd->GetList()->OMSetRenderTargets
	(
		rtvs_heap.size(),
		rtvs_heap.data(),
		false,
		&dsv_heap
	);
}

void Engine::DX12TextureBindingTask::UnbindMultiple(RenderPassTask* task_context, PrimitiveTexture* const* rtvs, const size_t rtv_count, PrimitiveTexture* dsv)
{
	DX12RenderPassTask* task = reinterpret_cast<DX12RenderPassTask*>(task_context);

	CommandPair* cmd = task->GetCurrentCommandList();
	DescriptorPtrImpl* heap = task->GetCurrentHeap();

	std::vector<D3D12_RESOURCE_BARRIER> transitions;
	transitions.reserve(rtv_count + 1);

	for (size_t i = 0; i < rtv_count; ++i)
	{
		DX12PrimitiveTexture* rtv = reinterpret_cast<DX12PrimitiveTexture*>(rtvs[i]);

		const auto& rtv_transition = CD3DX12_RESOURCE_BARRIER::Transition
		(
			static_cast<ID3D12Resource*>(rtvs[i]->GetPrimitiveTexture()),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COMMON
		);

		transitions.push_back(rtv_transition);
	}

	DX12PrimitiveTexture* native_dsv = reinterpret_cast<DX12PrimitiveTexture*>(dsv);

	const auto& dsv_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		static_cast<ID3D12Resource*>(native_dsv->GetPrimitiveTexture()),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_COMMON
	);

	transitions.push_back(dsv_transition);

	cmd->GetList()->ResourceBarrier(static_cast<UINT>(transitions.size()), transitions.data());
}
