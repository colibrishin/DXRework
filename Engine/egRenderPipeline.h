#pragma once
#include <BufferHelpers.h>
#include <filesystem>

#include "egCommon.hpp"
#include "egConstantBuffer.hpp"
#include "egD3Device.hpp"
#include "egDXCommon.h"

namespace Engine::Manager::Graphics
{
  using namespace Engine::Graphics;

  class RenderPipeline final : public Abstract::Singleton<RenderPipeline>
  {
  private:
    struct TempParamTicket
    {
      TempParamTicket(const CBs::ParamCB& previousParam)
        : previousParam(previousParam) {}

      ~TempParamTicket()
      {
        GetRenderPipeline().m_param_buffer_ = previousParam;
        GetRenderPipeline().m_param_buffer_data_.SetData(&previousParam);
      }

    private:
      const CBs::ParamCB previousParam;
    };

  public:
    explicit RenderPipeline(SINGLETON_LOCK_TOKEN)
      : m_material_buffer_() {}

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
    void SetMaterial(const CBs::MaterialCB& material_buffer);

    template <typename T>
    void SetParam(const T& v, const size_t slot)
    {
      m_param_buffer_.SetParam(slot, v);
    }

    [[nodiscard]] TempParamTicket SetParam(const Graphics::ParamBase& param)
    {
      return { m_param_buffer_ };
    }

    void DefaultRenderTarget() const;
    void DefaultViewport() const;

    void        DrawIndexedDeferred(UINT index_count);
    static void DrawIndexedInstancedDeferred(UINT index_count, UINT instance_count);

    RTVDSVHandlePair SetRenderTargetDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& rtv);
    RTVDSVHandlePair SetRenderTargetDeferred(
      const D3D12_CPU_DESCRIPTOR_HANDLE& rtv, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv
    );
    RTVDSVHandlePair SetRenderTargetDeferred(
         const UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE* srv, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv
       );
    void             SetRenderTargetDeferred(const RTVDSVHandlePair& rtv_dsv_pair) const;
    RTVDSVHandlePair SetDepthStencilOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv) const;
    void             SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const;
    void             SetShaderResources(UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data);
    void             SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const;

    static void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view);
    static void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view);
    static void UnbindVertexBuffer();
    static void UnbindIndexBuffer();

    void CopyBackBuffer(ID3D12Resource* resource) const;

    ID3D12RootSignature*  GetRootSignature() const;
    ID3D12DescriptorHeap* GetBufferHeap() const;
    ID3D12DescriptorHeap* GetSamplerHeap() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPURTVHandle(UINT index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSVHandle() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPURTVHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDSVHandle() const;
    D3D12_VIEWPORT              GetViewport();

    ID3D12RootSignature* GetRootSignature() const;
    void SetPSO(const StrongShader& Shader);

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RenderPipeline() = default;
    ~RenderPipeline() override;

    void PrecompileShaders();
    void InitializeDefaultPSO();
    void InitializeRootSignature();

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    D3D12_VIEWPORT m_viewport_{};
    D3D12_RECT    m_scissor_rect_{};
    
    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::TransformCB> m_transform_buffer_data_{};
    ConstantBuffer<CBs::MaterialCB> m_material_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

    std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
  };
} // namespace Engine::Manager::Graphics
