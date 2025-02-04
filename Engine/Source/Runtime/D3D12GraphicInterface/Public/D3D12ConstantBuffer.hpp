#pragma once
#include <directx/d3d12.h>

#include "CommandPair.h"
#include "DescriptorPtrImpl.h"

namespace Engine::Graphics
{
	class D3D12ConstantBufferTypeless : public ConstantBufferTypeless
	{
	public:
		~D3D12ConstantBufferTypeless() override;
		D3D12ConstantBufferTypeless()
			: m_b_dirty_(false),
			  m_data_(nullptr),
			  m_stride_(0),
			  m_alignment_(0) {}

		void                Create(const void* src_data, const size_t stride) override;
		void                SetData(const void* src_data, const size_t stride) override;
		[[nodiscard]] void* GetData() const override;
		void                Bind(const GraphicInterfaceContextPrimitive* context, const size_t slot) override
		{
			Bind(static_cast<CommandPair*>(context->commandList), static_cast<DescriptorPtrImpl*>(context->heap), slot);
		}

		void Bind(const CommandPair* cmd, const DescriptorPtrImpl* heap, const size_t slot);

		[[nodiscard]] UINT64 GetGPUAddress() const override
		{
			return m_buffer_->GetGPUVirtualAddress();
		}

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const
		{
			return m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart();
		}
	    
	private:
		bool m_b_dirty_;
		char* m_data_;

		size_t m_stride_;
		size_t m_alignment_;

		ComPtr<ID3D12DescriptorHeap> m_cpu_cbv_heap_;
		ComPtr<ID3D12Resource> m_upload_buffer_;
		ComPtr<ID3D12Resource> m_buffer_;
	};
}
