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
        reinterpret_cast<ParamBase&>(GetRenderPipeline().m_param_buffer_) = previousParam;
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
      m_param_buffer_.SetParam(slot, v);
      m_param_buffer_data_.SetData(&m_param_buffer_);
    }

    // Returns a ticket that will reset to the previous param when it goes out of scope.
    [[nodiscard]] TempParamTicket&& SetParam(const ParamBase& param);

    void DefaultRenderTarget() const;
    void DefaultViewport() const;

    void SetWireframeState() const;
    void SetFillState() const;
    void SetNoneCullState() const;
    void SetFrontCullState() const;

    void BindResource(
      const UINT slot, const D3D12_GPU_VIRTUAL_ADDRESS& address
    );

    void UnbindResource(const UINT slot);

    void        DrawIndexed(UINT index_count);
    static void DrawIndexedInstanced(UINT index_count, UINT instance_count);

    void        TargetDepthOnly(const D3D12_CPU_DESCRIPTOR_HANDLE * dsv_handle);
    static void SetViewport(const D3D12_VIEWPORT& viewport);

    ID3D12RootSignature*  GetRootSignature() const;
    ID3D12DescriptorHeap* GetCBHeap() const;
    static void           SetPSO(const StrongShader& Shader);

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RenderPipeline() = default;
    ~RenderPipeline() override;

    void PrecompileShaders();
    void InitializeDefaultPSO();
    void InitializeRootSignature();
    void InitializeRenderTargets();
    void InitializeDepthStencil();
    void InitializeHeaps();
    void InitializeStaticBuffers();

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_cb_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_srv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_uav_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_sampler_descriptor_heap_;

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
