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

  class Descriptors
  {
  public:
    Descriptors()
    {
      constexpr D3D12_DESCRIPTOR_HEAP_DESC buffer_heap_desc_gpu
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = g_total_engine_slots,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0
      };

      constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc_gpu
      {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        .NumDescriptors = g_max_sampler_slots,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0
      };

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateDescriptorHeap
         (
          &buffer_heap_desc_gpu, IID_PPV_ARGS(m_gpu_buffer_heap_.GetAddressOf())
         )
        );

      DX::ThrowIfFailed
        (
         GetD3Device().GetDevice()->CreateDescriptorHeap
         (
          &sampler_heap_desc_gpu, IID_PPV_ARGS(m_gpu_sampler_heap_.GetAddressOf())
         )
        );

      m_buffer_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

      m_sampler_descriptor_size_ = GetD3Device().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    ID3D12DescriptorHeap* GetBufferHeapGPU() const
    {
      return m_gpu_buffer_heap_.Get();
    }

    ID3D12DescriptorHeap* GetSamplerHeapGPU() const
    {
      return m_gpu_sampler_heap_.Get();
    }

    ID3D12DescriptorHeap** GetBufferAddress()
    {
      return m_gpu_buffer_heap_.GetAddressOf();
    }

    ID3D12DescriptorHeap** GetSamplerAddress()
    {
      return m_gpu_sampler_heap_.GetAddressOf();
    }

    void SetSampler(const D3D12_CPU_DESCRIPTOR_HANDLE& sampler, const UINT slot) const
    {
      const CD3DX12_CPU_DESCRIPTOR_HANDLE sampler_handle
        (
         m_gpu_sampler_heap_->GetCPUDescriptorHandleForHeapStart(),
         slot,
         m_sampler_descriptor_size_
        );

      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         sampler_handle,
         sampler,
         D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        );
    }

    void SetConstantBuffer(const D3D12_CPU_DESCRIPTOR_HANDLE& cbv, const UINT slot) const
    {
      const CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_handle
        (
         m_gpu_buffer_heap_->GetCPUDescriptorHandleForHeapStart(),
         g_cb_offset + slot,
         m_buffer_descriptor_size_
        );

      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         cbv_handle,
         cbv,
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
    }

    void SetShaderResource(const D3D12_CPU_DESCRIPTOR_HANDLE& srv_handle, const UINT slot) const
    {
      const CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
        (
         m_gpu_buffer_heap_->GetCPUDescriptorHandleForHeapStart(),
         g_srv_offset + slot,
         m_buffer_descriptor_size_
        );

      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         heap_handle,
         srv_handle,
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
    }

    void SetShaderResources(UINT slot, UINT count, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& data) const
    {
      CD3DX12_CPU_DESCRIPTOR_HANDLE heap_handle
        (
         m_gpu_buffer_heap_->GetCPUDescriptorHandleForHeapStart(),
         g_srv_offset + slot,
         m_buffer_descriptor_size_
        );

      for (INT i = 0; i < count; ++i)
      {
        const auto& current_handle = heap_handle.Offset(1, m_buffer_descriptor_size_);

        GetD3Device().GetDevice()->CopyDescriptorsSimple
          (
           1,
           current_handle,
           data[i],
           D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
          );
      }
    }

    void SetUnorderedAccess(const D3D12_CPU_DESCRIPTOR_HANDLE& uav, const UINT slot) const
    {
      const CD3DX12_CPU_DESCRIPTOR_HANDLE uav_handle
        (
         m_gpu_buffer_heap_->GetCPUDescriptorHandleForHeapStart(),
         g_uav_offset + slot,
         m_buffer_descriptor_size_
        );

      GetD3Device().GetDevice()->CopyDescriptorsSimple
        (
         1,
         uav_handle,
         uav,
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
    }

  private:
    ComPtr<ID3D12DescriptorHeap> m_gpu_buffer_heap_;
    ComPtr<ID3D12DescriptorHeap> m_gpu_sampler_heap_;

    UINT m_buffer_descriptor_size_;
    UINT m_sampler_descriptor_size_;

  };

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

    void DefaultRenderTarget(const eCommandList list) const;
    void DefaultViewport(const eCommandList list) const;

    void        DrawIndexed(const eCommandList list, UINT index_count);
    static void DrawIndexedInstancedDeferred(const eCommandList list, UINT index_count, UINT instance_count);

    RTVDSVHandlePair SetRenderTargetDeferred(const eCommandList list, const D3D12_CPU_DESCRIPTOR_HANDLE & rtv);
    RTVDSVHandlePair SetRenderTargetDeferred(
      const eCommandList list, const D3D12_CPU_DESCRIPTOR_HANDLE & rtv, const D3D12_CPU_DESCRIPTOR_HANDLE & dsv
    );
    RTVDSVHandlePair SetRenderTargetDeferred(
      const eCommandList            list, const UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE * srv, const
      D3D12_CPU_DESCRIPTOR_HANDLE & dsv
    );
    void             SetRenderTargetDeferred(const eCommandList list, const RTVDSVHandlePair & rtv_dsv_pair) const;
    RTVDSVHandlePair SetDepthStencilOnlyDeferred(const eCommandList list, const D3D12_CPU_DESCRIPTOR_HANDLE &dsv) const;

    RTVDSVHandlePair SetDepthStencilDeferred(const eCommandList list, const D3D12_CPU_DESCRIPTOR_HANDLE & dsv) const;
    void             TargetDepthOnlyDeferred(const eCommandList list, const D3D12_CPU_DESCRIPTOR_HANDLE * dsv_handle);
    static void      SetViewportDeferred(const eCommandList list, const D3D12_VIEWPORT & viewport);

    void CopyBackBuffer(ID3D12Resource* resource) const;

    ID3D12RootSignature*  GetRootSignature() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPURTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSVHandle() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPURTVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDSVHandle() const;
    D3D12_VIEWPORT              GetViewport();

    static void SetPSO(const StrongShader & Shader, const eCommandList list);

    UINT GetBufferDescriptorSize() const;
    UINT GetSamplerDescriptorSize() const;

    void UploadConstantBuffers();

    void SetRootSignature(const eCommandList list);
    void FallbackPSO(const eCommandList list);

    Descriptors& GetDescriptor();
    void SetDescriptor(const Descriptors& descriptor, const eCommandList list) const;

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

    ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
    ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

    UINT m_rtv_descriptor_size_ = 0;
    UINT m_dsv_descriptor_size_ = 0;
    UINT m_buffer_descriptor_size_ = 0;
    UINT m_sampler_descriptor_size_ = 0;

    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap_;
    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap_;
    Descriptors m_descriptor_;

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
