#include "D3D12PrimitiveSampler.h"
#include "D3D12GraphicInterface.h"

void Engine::D3D12PrimitiveSampler::Generate( eShaderSamplerAddress  addr,
                                              eShaderSamplerFunction function,
                                              eSamplerFilter         filter )
{
    IGraphicAPI &gi = g_graphic_accessor.GetInterface();
    ID3D12Device* dev = static_cast<ID3D12Device*>(gi.GetNativeInterface());

    constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        .NumDescriptors = 1,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0,
    };
    dev->CreateDescriptorHeap( &desc, IID_PPV_ARGS( m_sampler_heap_.GetAddressOf() ) );

    m_addr_ = static_cast<decltype( m_addr_ )>( addr );
    m_func_ = static_cast<decltype( m_func_ )>( function );
    m_filter_ = static_cast<decltype( m_filter_ )>( filter );

    D3D12_SAMPLER_DESC sd;
    sd.Filter           = m_filter_;
    sd.AddressU         = m_addr_;
    sd.AddressV         = m_addr_;
    sd.AddressW         = m_addr_;
    sd.MipLODBias       = 0.0f;
    sd.MaxAnisotropy    = 1;
    sd.ComparisonFunc   = m_func_;
    sd.BorderColor[ 0 ] = 0.0f;
    sd.BorderColor[ 1 ] = 0.0f;
    sd.BorderColor[ 2 ] = 0.0f;
    sd.BorderColor[ 3 ] = 0.0f;
    sd.MinLOD           = 0.0f;
    sd.MaxLOD           = D3D12_FLOAT32_MAX;
    
    dev->CreateSampler( &sd, m_sampler_heap_->GetCPUDescriptorHandleForHeapStart() );
    SetSampler( m_sampler_heap_.Get() );
}

bool Engine::D3D12PrimitiveSampler::IsValid()
{
    // weak assumption
    return m_sampler_heap_ != nullptr;
}

uint64_t Engine::D3D12PrimitiveSampler::GetCPUAddress() const
{
    return m_sampler_heap_->GetCPUDescriptorHandleForHeapStart().ptr;
}

uint64_t Engine::D3D12PrimitiveSampler::GetGPUAddress() const
{
    return m_sampler_heap_->GetGPUDescriptorHandleForHeapStart().ptr;
}
