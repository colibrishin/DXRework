#pragma once
#include <BufferHelpers.h>
#include <filesystem>

#include "egCommon.hpp"
#include "egD3Device.hpp"
#include "egDXCommon.h"

namespace Engine::Manager::Graphics
{
    class RenderPipeline final : public Abstract::Singleton<RenderPipeline>
    {
    public:
        RenderPipeline(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        ~RenderPipeline() override;

        void Initialize() override;
        void PreRender(const float& dt) override;

        void SetShader(Graphic::IShader* shader);

        void SetWorldMatrix(const TransformBuffer& matrix);
        void SetWorldMatrix(const TransformBuffer& matrix, eShaderType shader);
        void SetPerspectiveMatrix(const PerspectiveBuffer& matrix);

        void SetLight(UINT id, const Matrix& world, const Color& color);
        void SetCascadeBuffer(const CascadeShadowBuffer& shadow_buffer);
        void SetSpecularPower(float power);
        void SetSpecularColor(const Color& color);
        void SetClipPlane(const ClipPlaneBuffer& clip_plane_buffer);
        void SetRefraction(const RefractionBuffer& refraction_buffer);

        void SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology);

        void SetWireframeState() const;
        void SetFillState() const;
        void SetNoneCullState() const;
        void SetFrontCullState() const;

        TransformBuffer GetWorldMatrix() const;

        void BindLightBuffer(UINT light_count);
        void BindVertexBuffer(ID3D11Buffer* buffer);
        void BindIndexBuffer(ID3D11Buffer* buffer);

        void UpdateBuffer(ID3D11Buffer * buffer, const void * data, UINT size);

        void BindResource(
            eShaderResource resource,
            eShaderType     shader_type, ID3D11ShaderResourceView * texture);

        void InitializeShadowBuffer(GraphicShadowBuffer& buffer);
        void InitializeShadowProcessors();

        void DrawIndexed(UINT index_count);

        void TargetShadowMap(const GraphicShadowBuffer& buffer);
        void UseShadowMapViewport();
        void BindShadowMap(UINT size, ID3D11ShaderResourceView** p_shadow_maps);
        void UnbindShadowMap(UINT size);
        void BindShadowSampler();

        void ResetRenderTarget();
        void ResetViewport();
        void ResetShaders();
        void ResetShadowMap(ID3D11DepthStencilView* view);
        void ResetDepthStencilState();
        void BindCascadeBufferChunk(
            const CascadeShadowBufferChunk& cascade_shadow_buffer_chunk);
        void UnbindResource(eShaderResource shader_resource);
        void UnbindResource(eShaderResource shader_resource, eShaderType shader_type);

    private:
        friend class ToolkitAPI;
        friend class D3Device;

        void PrecompileShaders();
        void InitializeSamplers();

        void PreUpdate(const float& dt) override {}
        void Update(const float& dt) override {}
        void Render(const float& dt) override {}
        void FixedUpdate(const float& dt) override {}
        void PostRender(const float& dt) override {}
        void PostUpdate(const float& dt) override {}

    private:
        ConstantBuffer<PerspectiveBuffer> m_wvp_buffer_data_{};
        ConstantBuffer<TransformBuffer>   m_transform_buffer_data_{};

        TransformBuffer m_transform_buffer_{};
        LightBuffer    m_light_buffer_{};
        SpecularBuffer m_specular_buffer_{};

        ConstantBuffer<LightBuffer>              m_light_buffer_data{};
        ConstantBuffer<SpecularBuffer>           m_specular_buffer_data_{};
        ConstantBuffer<CascadeShadowBuffer>      m_shadow_buffer_data_{};
        ConstantBuffer<CascadeShadowBufferChunk> m_shadow_buffer_chunk_data_{};
        ConstantBuffer<RefractionBuffer>         m_refraction_buffer_data_{};
        ConstantBuffer<ClipPlaneBuffer>          m_clip_plane_buffer_data_{};

        std::map<eSampler, ID3D11SamplerState*> m_sampler_state_{};

        ComPtr<ID3D11BlendState>        m_blend_state_         = nullptr;
        ComPtr<ID3D11RasterizerState>   m_rasterizer_state_    = nullptr;
        ComPtr<ID3D11DepthStencilState> m_depth_stencil_state_ = nullptr;

        ComPtr<ID3D11SamplerState>      m_shadow_map_sampler_state_       = nullptr;
        ComPtr<ID3D11DepthStencilState> m_shadow_map_depth_stencil_state_ = nullptr;

        std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
    };
} // namespace Engine::Manager::Graphics
