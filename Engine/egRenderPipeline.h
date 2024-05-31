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
    void SetMaterial(const CBs::MaterialCB& material_buffer);

    template <typename T>
    void SetParam(const T& v, const size_t slot)
    {
      // todo: commit param value at the end of the execution
      m_param_buffer_.SetParam(slot, v);
      m_param_buffer_data_.SetData(&m_param_buffer_);
    }

    // Returns a ticket that will reset to the previous param when it goes out of scope.
    [[nodiscard]] TempParamTicket&& SetParam(const ParamBase& param);

    void DefaultRenderTarget() const;
    void DefaultViewport() const;

    void        DrawIndexedDeferred(UINT index_count);
    static void DrawIndexedInstancedDeferred(UINT index_count, UINT instance_count);

    RTVDSVHandlePair SetRenderTargetDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& rtv);
    RTVDSVHandlePair SetRenderTargetDeferred(
      const D3D12_CPU_DESCRIPTOR_HANDLE& rtv, const D3D12_CPU_DESCRIPTOR_HANDLE& dsv
    );
    void SetRenderTargetDeferred(const RTVDSVHandlePair& rtv_dsv_pair) const;
    RTVDSVHandlePair SetDepthStencilOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv) const;
    void             SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const;
    void             SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const;

    void        TargetDepthOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE * dsv_handle);
    static void SetViewportDeferred(const D3D12_VIEWPORT& viewport);

    void CopyBackBufferDeferred(ID3D12Resource* resource);

    ID3D12RootSignature*  GetRootSignature() const;
    ID3D12DescriptorHeap* GetCBHeap() const;
    ID3D12DescriptorHeap* GetSRVHeap() const;
    ID3D12DescriptorHeap* GetUAVHeap() const;
    ID3D12DescriptorHeap* GetSamplerHeap() const;

    static void           SetPSO(const StrongShader& Shader);

    UINT GetBufferDescriptorSize() const;
    UINT GetSamplerDescriptorSize() const;

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RenderPipeline() = default;
    ~RenderPipeline() override;

    void PrecompileShaders();
    void FallbackPSO();
    void InitializeRootSignature();
    void InitializeRenderTargets();
    void InitializeDepthStencil();
    void InitializeHeaps();
    void InitializeStaticBuffers();

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    UINT m_rtv_descriptor_size_ = 0;
    UINT m_dsv_descriptor_size_ = 0;
    UINT m_buffer_descriptor_size_ = 0;
    UINT m_sampler_descriptor_size_ = 0;
    
    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_cb_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_srv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_sampler_descriptor_heap_;

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

    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::TransformCB> m_transform_buffer_data_{};
    ConstantBuffer<CBs::MaterialCB> m_material_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

  };
} // namespace Engine::Manager::Graphics
