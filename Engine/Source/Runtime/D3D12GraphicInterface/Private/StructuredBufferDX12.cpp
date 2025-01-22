#include <directxtk12/Keyboard.h>

#include "CommandPair.h"
#include "Descriptors.h"

#include "../Public/StructuredBufferMemoryPoolDX12.hpp"

void Engine::Graphics::D3D12StructuredBufferTypeless::Clear()
{
	m_buffer_.Reset();
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionToSRV(const GraphicInterfaceContextPrimitive* context)
{
	auto* cmd = reinterpret_cast<ID3D12GraphicsCommandList1*>(context->commandList);

	const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
	);

	cmd->ResourceBarrier(1, &srv_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionToUAV(const GraphicInterfaceContextPrimitive* context)
{
	auto* cmd = reinterpret_cast<ID3D12GraphicsCommandList1*>(context->commandList);

	const auto& uav_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	cmd->ResourceBarrier(1, &uav_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::TransitionCommon(const GraphicInterfaceContextPrimitive* context)
{
	auto* cmd = reinterpret_cast<ID3D12GraphicsCommandList1*>(context->commandList);

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		m_current_state_,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->ResourceBarrier(1, &common_transition);
	m_current_state_ = D3D12_RESOURCE_STATE_COMMON;
}

void Engine::Graphics::D3D12StructuredBufferTypeless::CopySRVHeap(const GraphicInterfaceContextPrimitive* context, const UINT slot) const
{
	auto* heap = static_cast<DescriptorPtrImpl*>(context->heap);

	heap->SetShaderResource
	(
		m_srv_heap_->GetCPUDescriptorHandleForHeapStart(),
		slot
	);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::CopyUAVHeap(const GraphicInterfaceContextPrimitive* context, const UINT slot) const
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

void Engine::Graphics::D3D12StructuredBufferTypeless::Create(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride, const bool uav)
{
	if (size == 0)
	{
		size = 1;
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

void Engine::Graphics::D3D12StructuredBufferTypeless::SetData(const GraphicInterfaceContextPrimitive* context, UINT size, const void* src_ptr, const size_t stride)
{
	auto* cmd = reinterpret_cast<ID3D12GraphicsCommandList1*>(context->commandList);

	if (m_size_ < size)
	{
		Create(context, size, src_ptr, stride, m_uav_);
	}

	if (src_ptr == nullptr)
	{
		return;
	}

	char* data = nullptr;

	DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

	SIMDExtension::_mm256_memcpy(data, src_ptr, stride * size);

	m_upload_buffer_->Unmap(0, nullptr);

	cmd->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->ResourceBarrier(1, &common_transition);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::SetDataContainer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* const* src_ptr, const size_t stride)
{
	auto* cmd = reinterpret_cast<ID3D12GraphicsCommandList1*>(context->commandList);

	if (m_size_ < size)
	{
		Create(context, size, nullptr, stride, m_uav_);
	}

	if (src_ptr == nullptr)
	{
		return;
	}

	char* data = nullptr;

	DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

	for (UINT i = 0; i < size; ++i)
	{
		SIMDExtension::_mm256_memcpy(data + (i * stride), *(src_ptr + (i * stride)), stride);
	}

	m_upload_buffer_->Unmap(0, nullptr);

	cmd->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());

	const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
	(
		m_buffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON
	);

	cmd->ResourceBarrier(1, &common_transition);
}

void Engine::Graphics::D3D12StructuredBufferTypeless::GetData(const GraphicInterfaceContextPrimitive* context, UINT size, void* dst_ptr, const size_t stride)
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

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeSRV(UINT size, const size_t stride)
{
	const auto dev = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());

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
	const auto dev = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());
	
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
	const auto  dev         = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());

	if constexpr (m_uav_)
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

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeUploadBuffer(const GraphicInterfaceContextPrimitive* context, UINT size, const void* initial_data, const size_t stride)
{
	const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(stride * size);
	const auto  dev         = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());
	const auto  cmd         = static_cast<CommandPair*>(context->commandList);
	
	DX::ThrowIfFailed
	(
		dev->CreateCommittedResource
		(
			&upload_heap,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&buffer_desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_upload_buffer_.ReleaseAndGetAddressOf())
		)
	);

	const std::string  gen_type_name = typeid(DirectX::Keyboard::T).name();
	const std::wstring type_name(gen_type_name.begin(), gen_type_name.end());
	const std::wstring buffer_name = L"StructuredBuffer " + type_name;

	const std::wstring write_buffer_name = L"StructuredBuffer Write " + type_name;

	DX::ThrowIfFailed(m_upload_buffer_->SetName(write_buffer_name.c_str()));

	if (initial_data != nullptr)
	{
		char* data = nullptr;
		DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		SIMDExtension::_mm256_memcpy(data, initial_data, stride * size);
		m_upload_buffer_->Unmap(0, nullptr);

		cmd->GetList()->CopyResource
		(
			m_buffer_.Get(),
			m_upload_buffer_.Get()
		);

		const auto& common_transition = CD3DX12_RESOURCE_BARRIER::Transition
		(
			m_buffer_.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_COMMON
		);

		cmd->GetList()->ResourceBarrier(1, &common_transition);
	}
}

void Engine::Graphics::D3D12StructuredBufferTypeless::InitializeReadBuffer(UINT size, const size_t stride)
{
	const auto& readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(stride * size);
	const auto  dev         = static_cast<ID3D12Device2*>(g_graphic_interface.GetInterface().GetNativeInterface());

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
