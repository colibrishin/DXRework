#pragma once
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include "TypeLibrary.h"

#include "IGraphicAPI.h"
#include "ThrowIfFailed.h"

#include "SIMDExtension.hpp"

namespace Engine
{
    struct ENGINE_D3D12GRAPHICINTERFACE_API DescriptorPtrImpl : public IHeapBase
	{
	public:
		DescriptorPtrImpl(DescriptorPtrImpl&& other) noexcept;
		DescriptorPtrImpl& operator=(DescriptorPtrImpl&& other) noexcept;

		DescriptorPtrImpl(const DescriptorPtrImpl& other)            = delete;
		DescriptorPtrImpl& operator=(const DescriptorPtrImpl& other) = delete;

		~DescriptorPtrImpl() override;

		[[nodiscard]] bool IsValid() const;
		void               Release();

		[[nodiscard]] ID3D12DescriptorHeap* GetMainDescriptorHeap() const;
        [[nodiscard]] ID3D12DescriptorHeap* GetMainSamplerDescriptorHeap() const;

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const
		{
			return m_cpu_handle_;
		}

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const
		{
			return m_gpu_handle_;
		}

        void SetSampler( const Abstracts::Resource* shader, const eSampler slot ) const override;
        void SetSampler( const ISampler *sampler, const eSampler slot ) const override;
		void SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, UINT slot) const;
		void SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, UINT slot) const;
		void SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, UINT slot) const;
		void SetShaderResources(const Abstracts::Resource* const* textures, const UINT count, const UINT offset) const override;
		void SetShaderResources(UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data) const;
		void SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, UINT slot) const;

		void BindGraphic(const IGraphicContext* context) const override;
		void BindCompute(const IGraphicContext* context) const override;

	private:
		DescriptorPtrImpl();
		friend struct DescriptorHandlerBase;

        explicit DescriptorPtrImpl(
            DescriptorHandlerBase*             handler, const UINT64 heap_queue_offset, const INT64 segment_offset,
            const INT64                        element_offset, const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_handle, const D3D12_CPU_DESCRIPTOR_HANDLE& cpu_sampler_handle,
            const D3D12_GPU_DESCRIPTOR_HANDLE& gpu_sampler_handle
        )
			: m_handler_(handler),
			  m_segment_offset_(segment_offset),
			  m_element_offset_(element_offset),
			  m_heap_queue_offset_(heap_queue_offset),
			  m_cpu_handle_(cpu_handle),
			  m_gpu_handle_(gpu_handle),
			  m_cpu_sampler_handle_(cpu_sampler_handle),
			  m_gpu_sampler_handle_(gpu_sampler_handle) {}

	public:
        [[nodiscard]] UINT64 GetBufferHeapGPUAddress(const size_t offset) const override;
        [[nodiscard]] UINT64 GetSamplerHeapGPUAddress(const size_t offset) const override;
        
		[[nodiscard]] void* GetNativeHeap() override
		{
			return GetMainDescriptorHeap();
		}
		
		[[nodiscard]] void* GetNativeCPUHandle() override
		{
			return &m_cpu_handle_;
		}
		
		[[nodiscard]] void* GetNativeGPUHandle() override
		{
			return &m_gpu_handle_;
		}

	protected:
		DescriptorHandlerBase* m_handler_;
		INT64              m_segment_offset_;
		INT64              m_element_offset_;
		UINT64             m_heap_queue_offset_;

		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle_;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle_;

		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_sampler_handle_;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_sampler_handle_;
	};
}
