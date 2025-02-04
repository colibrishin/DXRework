#pragma once
#include "D3D12GraphicPrimitiveShader.h"

namespace Engine
{
    class ENGINE_D3D12GRAPHICINTERFACE_API D3D12RaytracingShader : public RaytracingPrimitiveShader
    {
    public:
        void                Generate(const Resources::RaytracingShader* shader, void* pipeline_signature) override;
        [[nodiscard]] void* GetShaderRecord(const size_t idx) const override;
        void                UpdateHitRecords(const byte_stream& hit_records) override;
        
    private:
        void InitializeLocalSignature(ID3D12Device5* dev);
        void InitializeShaderTable(const Resources::RaytracingShader* shader, ID3D12Device5* dev);

    private:
        ComPtr<ID3D12RootSignature>         m_local_root_signature_;
        ComPtr<ID3D12StateObject>           m_raytracing_pso_;
        ComPtr<ID3D12StateObjectProperties> m_raytracing_pso_properties_;
        ComPtr<ID3D12DescriptorHeap>        m_sampler_heap_;

        ComPtr<ID3D12Resource> m_shader_tables_[3];

        std::array<size_t, RAY_SHADER_REC_MAX> m_shader_record_sizes_{};
        
        size_t m_hit_shader_record_size_ = 0;
    };
}
