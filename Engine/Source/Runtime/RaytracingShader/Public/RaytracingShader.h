#pragma once
#if CFG_RAYTRACING
#include "ShaderBase.h"
#include "Resource.h"

#include "RaytracingShader.generated.h"

namespace Engine
{
    struct ALIGN(64) ShaderRecord
    {
        unsigned char shaderId[32];
    };

    struct ALIGN(64) HitShaderRecord : ShaderRecord
    {
        uint64_t addresses[RAYTRACING_LOCAL_SLOT_END];
    };
}

namespace Engine::Resources
{
    ECLASS( resource, serialize )
    class ENGINE_RAYTRACINGSHADER_API RaytracingShader : public ShaderBase
    {
        GENERATE_BODY
    public:
        RaytracingShader(
            const std::filesystem::path& path,
            const eShaderDomain domain,
            const std::array<bool, 4>& has_export,
            const std::wstring_view hit_group_name,
            const eSamplerFilter sampler_filter,
            const eShaderSamplerAddress sampler_addr_mode,
            const eShaderSamplerFunction sampler_func,
            const std::array<size_t, 3>& shader_record_sizes = { sizeof(ShaderRecord), sizeof(ShaderRecord), sizeof(ShaderRecord) });

        RaytracingShader(const RaytracingShader&);
        RaytracingShader& operator=(const RaytracingShader&);
        
        void PreUpdate(const float dt) override;
        void Update(const float dt) override;
        void PostUpdate(const float dt) override;
        void FixedUpdate(const float dt) override;
        void OnSerialized() override;

        [[nodiscard]] const std::array<bool,4>&                GetHasExport() const;
        [[nodiscard]] const std::array<unsigned long long, 3>& GetShaderRecordSizes() const;
        [[nodiscard]] std::wstring_view                        GetHitGroupName() const;
        [[nodiscard]] eSamplerFilter                           GetSamplerFilter() const;
        [[nodiscard]] eShaderSamplerAddress                    GetSamplerAddressMode() const;
        [[nodiscard]] eShaderSamplerFunction                   GetSamplerFunction() const;
        [[nodiscard]] IShaderBase&                     GetPrimitive() const override;

    protected:
        RaytracingShader();
        void             Load_INTERNAL() override;
        void             Unload_INTERNAL() override;

    private:
        EPROPERTY()
        std::array<bool, RAY_SHADER_MAX> m_has_export_ = { true, false, true, false };
        EPROPERTY()
        std::array<size_t, RAY_SHADER_REC_MAX> m_shader_record_sizes_ = { sizeof(ShaderRecord), sizeof(ShaderRecord), sizeof(ShaderRecord) };
        EPROPERTY()
        std::wstring m_hit_group_name_;
        EPROPERTY()
        eSamplerFilter m_sampler_filter_;
        EPROPERTY()
        eShaderSamplerAddress m_sampler_addr_mode_;
        EPROPERTY()
        eShaderSamplerFunction m_sampler_func_;

        Unique<IRaytracingShader> m_primitive_shader_;
    };   
}
#endif