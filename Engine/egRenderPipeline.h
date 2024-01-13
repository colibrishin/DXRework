#pragma once
#include <BufferHelpers.h>
#include <filesystem>

#include "egCommon.hpp"
#include "egD3Device.hpp"
#include "egDXCommon.h"

namespace Engine::Manager::Graphics
{
    using namespace Engine::Graphics;

    class RenderPipeline final : public Abstract::Singleton<RenderPipeline>
    {
    public:
        explicit RenderPipeline(SINGLETON_LOCK_TOKEN) {}

        void Initialize() override;
        void PreRender(const float& dt) override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void SetWorldMatrix(const CBs::TransformCB& matrix);
        void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);
        void SetGlobalStateBuffer(const CBs::GlobalStateCB& state);
        void SetMaterial(const CBs::MaterialCB& material_buffer);

        CBs::GlobalStateCB GetGlobalStateBuffer() const;

        void SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology);
        void SetDepthStencilState(ID3D11DepthStencilState* state);
        void SetRasterizerState(ID3D11RasterizerState* state);

        void SetWireframeState() const;
        void SetFillState() const;
        void SetNoneCullState() const;
        void SetFrontCullState() const;

        void BindVertexBuffer(ID3D11Buffer* buffer);
        void BindIndexBuffer(ID3D11Buffer* buffer);
        void UnbindVertexBuffer();
        void UnbindIndexBuffer();

        void BindResource(
            UINT slot, eShaderType shader_type, ID3D11ShaderResourceView ** texture);
        void BindResources(
            UINT slot, eShaderType shader_type, ID3D11ShaderResourceView ** textures, UINT size);
        void BindSampler(ID3D11SamplerState* sampler);

        void UnbindResource(UINT slot, eShaderType type);

        void DrawIndexed(UINT index_count);

        void TargetDepthOnly(ID3D11DepthStencilView * view, ID3D11DepthStencilState * state);
        void SetViewport(const D3D11_VIEWPORT& viewport);

        void DefaultRenderTarget();
        void DefaultViewport();
        void ResetShaders();
        void DefaultDepthStencilState();
        void DefaultRasterizerState();

    private:
        friend class ToolkitAPI;
        friend class D3Device;
        friend struct SingletonDeleter;
        RenderPipeline() = default;
        ~RenderPipeline() override;

        void PrecompileShaders();
        void InitializeSamplers();

        template <typename T>
        void BindConstantBuffer(const ConstantBuffer<T>& buffer, eShaderType target) const
        {
            GetD3Device().BindConstantBuffer(buffer, which_cb<T>::value, target);
        }

        CBs::GlobalStateCB m_global_state_{};

        ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
        ConstantBuffer<CBs::TransformCB>   m_transform_buffer_data_{};
        ConstantBuffer<CBs::GlobalStateCB> m_global_state_buffer_data_{};
        ConstantBuffer<CBs::MaterialCB>           m_material_buffer_data_{};

        std::map<eSampler, ID3D11SamplerState*> m_sampler_state_{};

        ComPtr<ID3D11BlendState>        m_blend_state_         = nullptr;
        ComPtr<ID3D11RasterizerState>   m_rasterizer_state_    = nullptr;
        ComPtr<ID3D11DepthStencilState> m_depth_stencil_state_ = nullptr;

        std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
    };
} // namespace Engine::Manager::Graphics
