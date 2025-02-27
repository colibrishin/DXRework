#pragma once
#include <directx/d3d12.h>
#include "IGraphicAPI.h"

namespace Engine
{
    struct D3D12PrimitiveSampler : public ISampler
    {
        void Generate( eShaderSamplerAddress addr, eShaderSamplerFunction function, eSamplerFilter filter ) override;
        virtual bool IsValid() override;

        uint64_t GetCPUAddress() const override;
        uint64_t GetGPUAddress() const override;
    private:
        D3D12_TEXTURE_ADDRESS_MODE m_addr_;
        D3D12_COMPARISON_FUNC      m_func_;
        D3D12_FILTER               m_filter_;

        ComPtr<ID3D12DescriptorHeap> m_sampler_heap_;
    };
}