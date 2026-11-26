#include "StructuredBufferDX12.hpp"

#include <algorithm>

#include <directxtk12/Keyboard.h>

#include "CommandPair.h"
#include "DescriptorPtrImpl.h"
#include "ThrowIfFailed.h"

#include "SIMDExtension.hpp"

void Engine::Graphics::D3D12StructuredBufferTypeless::Clear()
{
	// Lists of this frame may still use them; the next SetData makes new ones.
	KeepUntilFrameDone(std::move(m_buffer_));
	KeepUntilFrameDone(std::move(m_read_buffer_));
	m_upload_.Release();
	ReleaseReadbacks();
	m_size_          = 0;
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionToSRV(const IGraphicContext* context)
{
	if (m_current_state_ == D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
	{
		return; // identical before/after barriers invalidate the command list
	}

	auto* cmd = static_cast<CommandPair*>(context->commandList);

	const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		m_current_state_,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
	);

	cmd->GetList4()->ResourceBarrier(1, &srv_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionToUAV(const IGraphicContext* context)
{
	if (m_current_state_ == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		return; // identical before/after barriers invalidate the command list
	}

	auto* cmd = static_cast<CommandPair*>(context->commandList);

	const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		m_current_state_,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	cmd->GetList()->ResourceBarrier(1, &uav_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionCommon(const IGraphicContext* context)
{
	if (m_current_state_ == D3D12_RESOURCE_STATE_COMMON)
	{
		return; // identical before/after barriers invalidate the command list
	}

	auto* cmd = static_cast<CommandPair*>(context->commandList);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		m_current_state_,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList4()->ResourceBarrier(1, &common_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::CopySRVHeap(const IGraphicContext* context, const UINT slot) const
{
	auto* heap = static_cast<DescriptorPtrImpl*>(context->heap);

	heap->SetShaderResource
	(
		m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
		slot
	);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::CopyUAVHeap(const IGraphicContext* context, const UINT slot) const
{
	auto* heap = static_cast<DescriptorPtrImpl*>(context->heap);

	heap->SetUnorderedAccess
	(
		m_uav_heap_->GetCPUDescriptorHandleForHeapStart(),
		slot
	);
}

D3D12_GPU_VIRTUAL_ADDRESS Engine::Graphics::D3D12StructuredBufferTypeless::GetGPUAddress() const
{
	return m_buffer_->GetGPUVirtualAddress();
}

Engine::Graphics::D3D12StructuredBufferTypeless::~D3D12StructuredBufferTypeless()
{
	// Lists of this frame may still read or copy into them.
	KeepUntilFrameDone(std::move(m_buffer_));
	KeepUntilFrameDone(std::move(m_read_buffer_));
	ReleaseReadbacks();
}

void Engine::Graphics::D3D12StructuredBufferTypeless::ReleaseReadbacks()
{
	// Requested copies may still be on the GPU; pending ones are dropped.
	for (Readback& readback : m_readbacks_)
	{
		KeepUntilFrameDone(std::move(readback.buffer));
		readback = {};
	}
	m_ready_readback_ = SIZE_MAX;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::Create(const IGraphicContext* context, UINT size, const void* initial_data, const size_t stride, const bool uav)
{
	if (size == 0)
	{
		size = 1;
	}

	// Growing: lists recorded earlier in this frame may still use the old buffers.
	if (m_buffer_)
	{
		KeepUntilFrameDone(std::move(m_buffer_));
		KeepUntilFrameDone(std::move(m_read_buffer_));
		ReleaseReadbacks();
	}

	m_size_ = size;
	m_uav_ = uav;

	InitializeMainBuffer(size, stride);
	InitializeSRV(size, stride);
	if (m_uav_)
	{
		InitializeUAV(size, stride);
	}
	InitializeUploadBuffer(context, size, initial_data, stride);
	InitializeReadBuffer(size, stride);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::SetData(const IGraphicContext* context, UINT size, const void* src_ptr, const size_t stride, const bool uav)
{
	auto* cmd = static_cast<CommandPair*>(context->commandList);

	if (m_size_ < size)
	{
		// Without the data: it is copied below (Create would write this frame's upload buffer a first time).
		Create(context, size, nullptr, stride, uav);
	}

	if (src_ptr == nullptr)
	{
		return;
	}

	char* data = nullptr;
	ID3D12Resource* upload = UploadBuffer(size, stride);

	DX::ThrowIfFailed(upload->Map(0, nullptr, reinterpret_cast<void**>(&data)));

	SIMDExtension::_mm256_memcpy(data, src_ptr, stride * size);

	upload->Unmap(0, nullptr);

	// Only the written elements (at least one, so the copy always promotes the buffer to COPY_DEST for the barrier).
	cmd->GetList4()->CopyBufferRegion(m_buffer_.Get(), 0, upload, 0, static_cast<UINT64>((std::max)(size, 1u)) * stride);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList4()->ResourceBarrier(1, &common_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::SetDataContainer(const IGraphicContext* context, UINT size, const void* const* src_ptr, const size_t stride)
{
	auto* cmd = static_cast<CommandPair*>(context->commandList);

	if (m_size_ < size)
	{
		Create(context, size, nullptr, stride, m_uav_);
	}

	if (src_ptr == nullptr)
	{
		return;
	}

	char* data = nullptr;
	ID3D12Resource* upload = UploadBuffer(size, stride);

	DX::ThrowIfFailed(upload->Map(0, nullptr, reinterpret_cast<void**>(&data)));

	for (UINT i = 0; i < size; ++i)
	{
		SIMDExtension::_mm256_memcpy(data + (i * stride), *(src_ptr + i), stride);
	}

	upload->Unmap(0, nullptr);

	// Only the written elements (at least one, so the copy always promotes the buffer to COPY_DEST for the barrier).
	cmd->GetList4()->CopyBufferRegion(m_buffer_.Get(), 0, upload, 0, static_cast<UINT64>((std::max)(size, 1u)) * stride);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList4()->ResourceBarrier(1, &common_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::SetDataPointerContainer(
	const IGraphicContext* context, UINT size, const void* const* src_ptr, const size_t stride
)
{
	auto* cmd = static_cast<CommandPair*>(context->commandList);

	if (m_size_ < size)
	{
		Create(context, size, nullptr, stride, m_uav_);
	}

	if (src_ptr == nullptr)
	{
		return;
	}

	char* data = nullptr;
	ID3D12Resource* upload = UploadBuffer(size, stride);

	DX::ThrowIfFailed(upload->Map(0, nullptr, reinterpret_cast<void**>(&data)));

	for (UINT i = 0; i < size; ++i)
	{
		SIMDExtension::_mm256_memcpy(data + (i * stride), *(src_ptr + i), stride);
	}

	upload->Unmap(0, nullptr);

	// Only the written elements (at least one, so the copy always promotes the buffer to COPY_DEST for the barrier).
	cmd->GetList4()->CopyBufferRegion(m_buffer_.Get(), 0, upload, 0, static_cast<UINT64>((std::max)(size, 1u)) * stride);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList4()->ResourceBarrier(1, &common_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::GetData(const IGraphicContext* context, UINT size, void* dst_ptr, const size_t stride)
{
	auto* cmd = reinterpret_cast<CommandPair*>(context->commandList);
	cmd->SoftReset();

	const auto& copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList()->ResourceBarrier(1, &copy_barrier);
	cmd->GetList()->CopyResource
	(
		m_read_buffer_.Get(),
		m_buffer_.Get()
	);
	cmd->GetList()->ResourceBarrier(1, &revert_barrier);
	cmd->Execute();

	char* data = nullptr;
	DX::ThrowIfFailed(m_read_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
	SIMDExtension::_mm256_memcpy(dst_ptr, data, stride * size);
	m_read_buffer_->Unmap(0, nullptr);
}

bool Engine::Graphics::D3D12StructuredBufferTypeless::RequestData(const IGraphicContext* context, const UINT size, const size_t stride, const uint64_t tag)
{
	Readback& readback = m_readbacks_[m_next_readback_ % m_readbacks_.size()];
	if (readback.buffer && !FrameDone(readback.frame))
	{
		return false; // more requests than frames in flight + 1: the oldest copy is still on the GPU
	}

	const UINT64 bytes = static_cast<UINT64>((std::min)(size, m_size_)) * stride;
	if (bytes == 0 || !m_buffer_)
	{
		return false;
	}
	if (readback.bytes < bytes)
	{
		KeepUntilFrameDone(std::move(readback.buffer));
		const auto  heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		const auto  desc = CD3DX12_RESOURCE_DESC::Buffer(bytes);
		const auto  dev  = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());
		DX::ThrowIfFailed(dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
		                                               nullptr, IID_PPV_ARGS(readback.buffer.ReleaseAndGetAddressOf())));
		DX::ThrowIfFailed(readback.buffer->SetName(L"StructuredBuffer Readback"));
		readback.bytes = bytes;
	}

	TransitionCommon(context); // the copy starts from COMMON
	auto* cmd = static_cast<CommandPair*>(context->commandList);
	const auto& copy_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	const auto& revert_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
	cmd->GetList()->ResourceBarrier(1, &copy_barrier);
	cmd->GetList()->CopyBufferRegion(readback.buffer.Get(), 0, m_buffer_.Get(), 0, bytes);
	cmd->GetList()->ResourceBarrier(1, &revert_barrier);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;

	readback.frame  = RecordingFrame();
	readback.tag    = tag;
	readback.copied = bytes;
	m_next_readback_ += 1;
	return true;
}

uint64_t Engine::Graphics::D3D12StructuredBufferTypeless::ReadyData()
{
	size_t newest = SIZE_MAX;
	for (size_t i = 0; i < m_readbacks_.size(); ++i)
	{
		const Readback& readback = m_readbacks_[i];
		if (readback.tag && FrameDone(readback.frame) && (newest == SIZE_MAX || readback.tag > m_readbacks_[newest].tag))
		{
			newest = i;
		}
	}
	if (newest == SIZE_MAX)
	{
		return 0;
	}
	for (Readback& readback : m_readbacks_)
	{
		if (readback.tag && readback.tag < m_readbacks_[newest].tag && FrameDone(readback.frame))
		{
			readback.tag = 0; // older than the newest finished copy
		}
	}
	m_ready_readback_ = newest;
	return m_readbacks_[newest].tag;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::ReadData(const uint64_t tag, const UINT size, void* dst_ptr, const size_t stride)
{
	if (m_ready_readback_ == SIZE_MAX || m_readbacks_[m_ready_readback_].tag != tag)
	{
		return;
	}
	Readback& readback = m_readbacks_[m_ready_readback_];
	const UINT64 bytes = (std::min)(readback.copied, static_cast<UINT64>(size) * stride);
	const D3D12_RANGE range{ 0, static_cast<SIZE_T>(bytes) };
	char* data = nullptr;
	DX::ThrowIfFailed(readback.buffer->Map(0, &range, reinterpret_cast<void**>(&data)));
	SIMDExtension::_mm256_memcpy(dst_ptr, data, bytes);
	constexpr D3D12_RANGE written{ 0, 0 };
	readback.buffer->Unmap(0, &written);
	readback.tag      = 0;
	m_ready_readback_ = SIZE_MAX;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeSRV(UINT size, const size_t stride)
{
	const auto dev = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());

	constexpr D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc
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
			&srv_heap_desc,
			IID_PPV_ARGS(m_srv_heap_.ReleaseAndGetAddressOf())
		)
	);

	const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc
	{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
		{
			.FirstElement = 0,
			.NumElements = size,
			.StructureByteStride = static_cast<UINT>(stride),
			.Flags = D3D12_BUFFER_SRV_FLAG_NONE
		}
	};

	dev->CreateShaderResourceView
	(
		m_buffer_.Get(),
		&srv_desc,
		m_srv_heap_->GetCPUDescriptorHandleForHeapStart()
	);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeUAV(UINT size, const size_t stride)
{
	const auto dev = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());
	
	constexpr D3D12_DESCRIPTOR_HEAP_DESC uav_heap_desc
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
			&uav_heap_desc,
			IID_PPV_ARGS(m_uav_heap_.ReleaseAndGetAddressOf())
		)
	);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = size;
	uav_desc.Buffer.StructureByteStride = stride;
	uav_desc.Buffer.CounterOffsetInBytes = 0;
	uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	dev->CreateUnorderedAccessView
	(
		m_buffer_.Get(),
		nullptr,
		&uav_desc,
		m_uav_heap_->GetCPUDescriptorHandleForHeapStart()
	);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeMainBuffer(UINT size, const size_t stride)
{
	const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto        buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(stride * size);
	const auto  dev         = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());

	if (m_uav_)
	{
		buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	const std::wstring buffer_name = L"StructuredBuffer";

	DX::ThrowIfFailed
	(
		dev->CreateCommittedResource
		(
			&default_heap,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&buffer_desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_buffer_.ReleaseAndGetAddressOf())
		)
	);

	DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));
}

ID3D12Resource* Engine::Graphics::D3D12StructuredBufferTypeless::UploadBuffer(const UINT size, const size_t stride)
{
	const auto dev = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());
	return m_upload_.Get(dev, static_cast<UINT64>((std::max)(size, 1u)) * stride, D3D12_RESOURCE_STATE_COMMON, L"StructuredBuffer Write");
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeUploadBuffer(const IGraphicContext* context, UINT size, const void* initial_data, const size_t stride)
{
	if (initial_data == nullptr)
	{
		return; // the upload buffer of a frame is made when that frame writes
	}

	const auto      cmd    = static_cast<CommandPair*>(context->commandList);
	ID3D12Resource* upload = UploadBuffer(size, stride);

	char* data = nullptr;
	DX::ThrowIfFailed(upload->Map(0, nullptr, reinterpret_cast<void**>(&data)));
	SIMDExtension::_mm256_memcpy(data, initial_data, stride * size);
	upload->Unmap(0, nullptr);

	cmd->GetList()->CopyBufferRegion(m_buffer_.Get(), 0, upload, 0, static_cast<UINT64>(size) * stride);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->GetList()->ResourceBarrier(1, &common_transition);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeReadBuffer(UINT size, const size_t stride)
{
	const auto& readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(stride * size);
	const auto  dev         = static_cast<ID3D12Device2*>(g_graphic_accessor.GetInterface().GetNativeInterface());

	DX::ThrowIfFailed
	(
		dev->CreateCommittedResource
		(
			&readback_heap,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&buffer_desc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_read_buffer_.ReleaseAndGetAddressOf())
		)
	);
}

void* Engine::Graphics::D3D12StructuredBufferTypeless::GetResource() const
{
    return m_buffer_.Get();
}
