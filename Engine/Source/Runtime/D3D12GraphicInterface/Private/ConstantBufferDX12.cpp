#include "../Public/ConstantBufferDX12.hpp"

#include "SIMDExtension/Public/SIMDExtension.hpp"

Engine::Graphics::D3D12ConstantBufferTypeless::~D3D12ConstantBufferTypeless()
{
	delete[] m_data_;
}

void Engine::Graphics::D3D12ConstantBufferTypeless::Create(const void* src_data, const size_t stride)
{
	m_stride_ = stride;
	m_alignment_ = (stride + 255) & ~255;

	GraphicInterface&                        gi      = GraphicInterfaceAccessor::GetInterface();
	const auto                               dev     = static_cast<ID3D12Device2*>(gi.GetNativeInterface());
	const GraphicInterfaceContextReturnType& context = gi.GetNewContext
			(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"D3D12ConstantBuffer Initialization");
	const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

	const auto cmd = static_cast<CommandPair*>(primitive.commandList);
	cmd->SoftReset();

	const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const auto& cb_desc      = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_);

	DX::ThrowIfFailed
			(
			 dev->CreateCommittedResource
			 (
			  &default_heap,
			  D3D12_HEAP_FLAG_NONE,
			  &cb_desc,
			  D3D12_RESOURCE_STATE_COPY_DEST,
			  nullptr,
			  IID_PPV_ARGS(m_buffer_.GetAddressOf())
			 )
			);

	const std::wstring buffer_name   = L"Constant Buffer Creation";
	DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));

	const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_);

	DX::ThrowIfFailed
			(
			 dev->CreateCommittedResource
			 (
			  &upload_heap,
			  D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			  &buffer_desc,
			  D3D12_RESOURCE_STATE_GENERIC_READ,
			  nullptr,
			  IID_PPV_ARGS(m_upload_buffer_.GetAddressOf())
			 )
			);

	const std::wstring upload_buffer_name = L" Constant Buffer Upload Buffer";

	DX::ThrowIfFailed(m_upload_buffer_->SetName(upload_buffer_name.c_str()));

	if (src_data != nullptr)
	{
		char* data = nullptr;

		DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		SIMDExtension::_mm256_memcpy(data, src_data, stride);
		m_upload_buffer_->Unmap(0, nullptr);

		cmd->GetList()->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
	}

	const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
			(
			 m_buffer_.Get(),
			 D3D12_RESOURCE_STATE_COPY_DEST,
			 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			);

	cmd->GetList()->ResourceBarrier(1, &cb_trans);

	const D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc
	{
		.BufferLocation = m_buffer_->GetGPUVirtualAddress(),
		.SizeInBytes = static_cast<UINT>(m_alignment_)
	};

	cmd->Execute();

	constexpr D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	DX::ThrowIfFailed
			(
			 dev->CreateDescriptorHeap
			 (&cbv_heap_desc, IID_PPV_ARGS(m_cpu_cbv_heap_.GetAddressOf()))
			);

	dev->CreateConstantBufferView
			(
			 &cbv_desc,
			 m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart()
			);

	m_b_dirty_ = false;

	m_data_ = new char[stride];

	if (src_data)
	{
		SIMDExtension::_mm256_memcpy(m_data_, src_data, stride);	
	}
}

void Engine::Graphics::D3D12ConstantBufferTypeless::SetData(const void* src_data, const size_t stride)
{
	if (m_data_ == nullptr)
	{
		Create(src_data, stride);
		return;
	}

	if (src_data != nullptr)
	{
		SIMDExtension::_mm256_memcpy(m_data_, src_data, stride);
	}

	m_b_dirty_ = true;
}

void* Engine::Graphics::D3D12ConstantBufferTypeless::GetData() const
{
	return m_data_;
}

void Engine::Graphics::D3D12ConstantBufferTypeless::Bind(const CommandPair* cmd, const DescriptorPtrImpl* heap, const size_t slot)
{
	assert(m_buffer_);
	assert(m_cpu_cbv_heap_);

	if (m_b_dirty_)
	{
		const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_buffer_.Get(),
				 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				 D3D12_RESOURCE_STATE_COPY_DEST
				);

		char* data = nullptr;
		DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		SIMDExtension::_mm256_memcpy(data, m_data_, m_stride_);
		m_upload_buffer_->Unmap(0, nullptr);

		const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
				(
				 m_buffer_.Get(),
				 D3D12_RESOURCE_STATE_COPY_DEST,
				 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
				);

		cmd->GetList()->ResourceBarrier(1, &copy_trans);
		cmd->GetList()->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
		cmd->GetList()->ResourceBarrier(1, &cb_trans);

		m_b_dirty_ = false;
	}

	if (heap == nullptr)
	{
		return;
	}

	heap->SetConstantBuffer(m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart(), slot);
}
