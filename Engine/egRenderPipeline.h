#pragma once
#include <BufferHelpers.h>
#include <filesystem>

#include "egCommon.hpp"
#include "egConstantBuffer.hpp"
#include "egD3Device.hpp"
#include "egDXCommon.h"
#include "egDescriptors.h"

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
      m_param_buffer_data_.SetData(&m_param_buffer_);
    }

    [[nodiscard]] TempParamTicket SetParam(const Graphics::ParamBase& param)
    {
      return { m_param_buffer_ };
    }

    void DefaultRenderTarget(ID3D12GraphicsCommandList1 * list) const;
    void DefaultViewport(ID3D12GraphicsCommandList1 * list) const;
    void DefaultScissorRect(ID3D12GraphicsCommandList1 * list) const;

    void CopyBackBuffer(ID3D12Resource * resource) const;

    ID3D12RootSignature*  GetRootSignature() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPURTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSVHandle() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPURTVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDSVHandle() const;
    D3D12_VIEWPORT              GetViewport() const;
    D3D12_RECT                  GetScissorRect() const;

    static void SetPSO(const StrongShader& Shader, const eCommandList list);

    [[nodiscard]] DescriptorPtr AcquireHeapSlot();

    UINT GetBufferDescriptorSize() const;
    UINT GetSamplerDescriptorSize() const;

    void BindConstantBuffers(const DescriptorPtr & heap);

    void FallbackPSO(ID3D12GraphicsCommandList1* list);

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RenderPipeline() = default;
    ~RenderPipeline() override;

    void PrecompileShaders();
    void InitializeRootSignature();
    void InitializeRenderTargets();
    void InitializeDepthStencil();
    void InitializeNullDescriptors();
    void InitializeHeaps();
    void InitializeStaticBuffers();
    void InitializeViewport();

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    UINT m_rtv_descriptor_size_ = 0;
    UINT m_dsv_descriptor_size_ = 0;
    UINT m_buffer_descriptor_size_ = 0;
    UINT m_sampler_descriptor_size_ = 0;

    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_;
    DescriptorHandler m_descriptor_handler_;

    ComPtr<ID3D12DescriptorHeap> m_null_srv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_null_sampler_heap_;
    ComPtr<ID3D12DescriptorHeap> m_null_cbv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_null_uav_heap_;
    ComPtr<ID3D12DescriptorHeap> m_null_rtv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_null_dsv_heap_;

    std::vector<ComPtr<ID3D12Resource>> m_render_targets_;
    ComPtr<ID3D12Resource> m_depth_stencil_;

    D3D12_VIEWPORT m_viewport_{};
    D3D12_RECT    m_scissor_rect_{};

    CBs::PerspectiveCB m_wvp_buffer_;
    CBs::TransformCB   m_transform_buffer_;
    CBs::MaterialCB    m_material_buffer_;
    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::TransformCB> m_transform_buffer_data_{};
    ConstantBuffer<CBs::MaterialCB> m_material_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

    StrongShader m_fallback_shader_;

  };
} // namespace Engine::Manager::Graphics
