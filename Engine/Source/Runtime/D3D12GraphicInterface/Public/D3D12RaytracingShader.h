#pragma once
#if CFG_RAYTRACING
#include "D3D12GraphicPrimitiveShader.h"

namespace Engine
{
    inline const wchar_t* g_raytracing_export_names[] =
    {
        L"raygen_main",
        L"any_hit_main",
        L"closest_hit_main",
        L"miss_main"
    };
    
    class ENGINE_D3D12GRAPHICINTERFACE_API D3D12RaytracingShader : public IRaytracingShader
    {
    public:
        void                Generate(const Resources::RaytracingShader* shader, void* pipeline_signature) override;
        [[nodiscard]] void* GetShaderRecord(const size_t idx) const override;

        void UpdateShaderRecords(eRaytracingShaderRecordType type, const byte_stream& records) override;
        
    private:
        void InitializeLocalSignature(ID3D12Device5* dev);
        void InitializeShaderTable(const Resources::RaytracingShader* shader, ID3D12Device5* dev);
        
    private:
        ComPtr<ID3D12RootSignature>         m_local_root_signature_;
        ComPtr<ID3D12StateObject>           m_raytracing_pso_;
        ComPtr<ID3D12StateObjectProperties> m_raytracing_pso_properties_;
        ComPtr<ID3D12DescriptorHeap>        m_sampler_heap_;

        std::wstring_view m_hit_group_name_{};
        ComPtr<ID3D12Resource> m_shader_tables_[RAY_SHADER_REC_MAX];
        
        size_t m_allocated_shader_record_size_[RAY_SHADER_REC_MAX]{};
    };
}
#endif