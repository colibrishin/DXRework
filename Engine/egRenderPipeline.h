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
    explicit RenderPipeline(SINGLETON_LOCK_TOKEN) {}

    void Initialize() override;
    void PreRender(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);

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

    void DefaultRenderTarget(const CommandPair & cmd) const;
    void DefaultViewport(const CommandPair & cmd) const;
    void DefaultScissorRect(const CommandPair & cmd) const;

    void CopyBackBuffer(const CommandPair & cmd, ID3D12Resource * resource) const;

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

    RTVDSVHandlePair SetDepthStencilDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE& dsv) const;
    void        TargetDepthOnlyDeferred(const D3D12_CPU_DESCRIPTOR_HANDLE * dsv_handle);
    static void SetViewportDeferred(const D3D12_VIEWPORT& viewport);

    void CopyBackBuffer(ID3D12Resource* resource) const;

    ID3D12RootSignature*  GetRootSignature() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPURTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSVHandle() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPURTVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDSVHandle() const;
    D3D12_VIEWPORT              GetViewport() const;
    D3D12_RECT                  GetScissorRect() const;

    static void SetPSO(const CommandPair & cmd, const StrongShader & Shader);

    [[nodiscard]] DescriptorPtr AcquireHeapSlot();
    [[nodiscard]] bool          IsHeapAvailable() const;

    UINT GetBufferDescriptorSize() const;
    UINT GetSamplerDescriptorSize() const;

    void BindConstantBuffers(const DescriptorPtr & heap);

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

    void SetRootSignature();
    void SetHeaps();

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    UINT m_rtv_descriptor_size_ = 0;
    UINT m_dsv_descriptor_size_ = 0;
    UINT m_buffer_descriptor_size_ = 0;
    UINT m_sampler_descriptor_size_ = 0;

    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_;
    std::mutex m_descriptor_mutex_;
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
    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

    StrongShader m_fallback_shader_;

  };
} // namespace Engine::Manager::Graphics
