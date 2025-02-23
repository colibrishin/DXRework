#include "../Public/DescriptorPtrImpl.h"

#include "CommandPair.h"
#include "DescriptorHandler.hpp"

namespace Engine
{
    DescriptorPtrImpl::DescriptorPtrImpl()
        : m_handler_(nullptr),
          m_segment_offset_(-1),
          m_element_offset_(-1),
          m_heap_queue_offset_(-1),
          m_cpu_handle_(),
          m_gpu_handle_(),
          m_cpu_sampler_handle_(),
          m_gpu_sampler_handle_() {}

    UINT64 DescriptorPtrImpl::GetBufferHeapGPUAddress(const size_t offset) const
    {
        return m_handler_->GetBufferHeapGPUAddress(m_gpu_handle_, offset);
    }
    
    UINT64 DescriptorPtrImpl::GetSamplerHeapGPUAddress(const size_t offset) const
    {
        return m_handler_->GetSamplerHeapGPUAddress(m_gpu_sampler_handle_, offset);
    }

    DescriptorPtrImpl::DescriptorPtrImpl(DescriptorPtrImpl&& other) noexcept
    {
        m_handler_                 = std::move(other.m_handler_);
        m_segment_offset_          = std::move(other.m_segment_offset_);
        m_element_offset_          = std::move(other.m_element_offset_);
        m_heap_queue_offset_       = std::move(other.m_heap_queue_offset_);
        m_cpu_handle_              = std::move(other.m_cpu_handle_);
        m_gpu_handle_              = std::move(other.m_gpu_handle_);
        m_cpu_sampler_handle_      = std::move(other.m_cpu_sampler_handle_);
        m_gpu_sampler_handle_      = std::move(other.m_gpu_sampler_handle_);
    }

    DescriptorPtrImpl& DescriptorPtrImpl::operator=(DescriptorPtrImpl&& other) noexcept
    {
        if (this != &other)
        {
            m_handler_                 = std::move(other.m_handler_);
            m_segment_offset_          = std::move(other.m_segment_offset_);
            m_element_offset_          = std::move(other.m_element_offset_);
            m_heap_queue_offset_       = std::move(other.m_heap_queue_offset_);
            m_cpu_handle_              = std::move(other.m_cpu_handle_);
            m_gpu_handle_              = std::move(other.m_gpu_handle_);
            m_cpu_sampler_handle_      = std::move(other.m_cpu_sampler_handle_);
            m_gpu_sampler_handle_      = std::move(other.m_gpu_sampler_handle_);
        }

        return *this;
    }

    DescriptorPtrImpl::~DescriptorPtrImpl() { Release(); }

    bool DescriptorPtrImpl::IsValid() const { return m_handler_ && m_handler_->IsValid(this); }

    void DescriptorPtrImpl::Release()
    {
        if (IsValid())
        {
            m_handler_->Release(*this);
            m_cpu_handle_              = {};
            m_cpu_sampler_handle_      = {};
            m_gpu_handle_              = {};
            m_gpu_sampler_handle_      = {};

            m_element_offset_    = -1;
            m_heap_queue_offset_ = -1;
            m_segment_offset_    = -1;
        }
    }

    ID3D12DescriptorHeap* DescriptorPtrImpl::GetMainDescriptorHeap() const
    {
        if (!IsValid()) { return nullptr; }

        return m_handler_->GetMainDescriptorHeap(m_heap_queue_offset_);
    }

    ID3D12DescriptorHeap* DescriptorPtrImpl::GetMainSamplerDescriptorHeap() const
    {
        if (!IsValid()) { return nullptr; }

        return m_handler_->GetMainSamplerDescriptorHeap(m_heap_queue_offset_);
    }

    void DescriptorPtrImpl::SetSampler(const Resources::ShaderBase* shader, const eSampler slot) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetSampler(
            m_cpu_sampler_handle_,
            static_cast<ID3D12DescriptorHeap*>(shader->GetPrimitive().GetNativeSampler())->GetCPUDescriptorHandleForHeapStart(),
            slot);
    }

    void DescriptorPtrImpl::SetSampler( const PrimitiveSampler *sampler, const eSampler slot ) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetSampler(
            m_cpu_sampler_handle_,
            static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(sampler->GetCPUAddress()),
            slot);
    }

    void DescriptorPtrImpl::SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, const UINT slot) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetSampler(m_cpu_sampler_handle_, sampler, slot);
    }

    void DescriptorPtrImpl::SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, const UINT slot) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetConstantBuffer(m_cpu_handle_, cbv, slot);
    }

    void DescriptorPtrImpl::SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetShaderResource(m_cpu_handle_, srv_handle, slot);
    }

    void DescriptorPtrImpl::SetShaderResources(
        const Resources::Texture* const* textures, const UINT count, const UINT offset
    ) const
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;

        for (size_t i = 0; i < count; ++i)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = static_cast<D3D12PrimitiveTexture*>(textures[i]->GetPrimitiveTexture())
                                                 ->GetSrv()->GetCPUDescriptorHandleForHeapStart();
            handles.push_back(handle);
        }

        SetShaderResources(offset, count, handles);
    }

    void DescriptorPtrImpl::SetShaderResources(
        UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data
    ) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetShaderResources(m_cpu_handle_, slot, count, data);
    }

    void DescriptorPtrImpl::SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const
    {
        if (!IsValid()) { return; }
        m_handler_->SetUnorderedAccess(m_cpu_handle_, uav, slot);
    }

    void DescriptorPtrImpl::BindGraphic(const GraphicInterfaceContextPrimitive* context) const
    {
        if (!IsValid()) { return; }

        m_handler_->BindGraphic
            (context, GetMainDescriptorHeap(), GetMainSamplerDescriptorHeap(), m_gpu_handle_, m_gpu_sampler_handle_);
    }

    void DescriptorPtrImpl::BindCompute(const GraphicInterfaceContextPrimitive* context) const
    {
        if (!IsValid()) { return; }

        m_handler_->BindCompute
            (context, GetMainDescriptorHeap(), GetMainSamplerDescriptorHeap(), m_gpu_handle_, m_gpu_sampler_handle_);
    }
}
