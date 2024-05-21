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
      m_param_buffer_.SetParam(slot, v);
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
    ID3D12DescriptorHeap* GetBufferHeap() const;
    ID3D12DescriptorHeap* GetSamplerHeap() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPURTVHandle(UINT index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDSVHandle() const;

    ID3D12RootSignature* GetRootSignature() const;

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
    
    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::TransformCB> m_transform_buffer_data_{};
    ConstantBuffer<CBs::MaterialCB> m_material_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

    std::vector<D3D11_INPUT_ELEMENT_DESC> m_input_element_desc_;
  };
} // namespace Engine::Manager::Graphics
