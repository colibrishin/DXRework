#pragma once
#include <BufferHelpers.h>

#include "egCommon.hpp"
#include "egConstantBuffer.hpp"
#include "egDescriptors.h"

namespace Engine::Manager::Graphics
{
  using namespace Engine::Graphics;

  class RaytracingPipeline final : public Abstract::Singleton<RaytracingPipeline>
  {
  public:
    explicit RaytracingPipeline(SINGLETON_LOCK_TOKEN) {}

    void Initialize() override;
    void PreRender(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void BuildTLAS(
      ID3D12GraphicsCommandList5*           cmd,
      const std::vector<AccelStructBuffer>& blases,
      const std::vector<SBs::InstanceSB>&   instances
    );

    ID3D12Device5* GetDevice() const;

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RaytracingPipeline() = default;
    ~RaytracingPipeline() override;

    void InitializeViewport();
    void InitializeInterface();
    void InitializeDescriptorHeaps();
    void InitializeRaytracingPSO();
    void InitializeOutputBuffer();

    ComPtr<ID3D12Device5> m_device_;
    ComPtr<ID3D12GraphicsCommandList4> m_command_list_;

    ComPtr<ID3D12PipelineState> m_pipeline_state_;
    ComPtr<ID3D12DescriptorHeap> m_raytracing_buffer_heap_;
    ComPtr<ID3D12DescriptorHeap> m_raytracing_sampler_heap_;

    ComPtr<ID3D12Resource> m_output_buffer_;

    ComPtr<ID3D12DescriptorHeap> m_output_uav_heap_;
    StrongDescriptorPtr m_raytracing_heap_;

    UINT m_buffer_descriptor_size_ = 0;

    AccelStructBuffer m_tlas_;

    D3D12_VIEWPORT m_viewport_{};
    D3D12_RECT    m_scissor_rect_{};

    CBs::PerspectiveCB m_wvp_buffer_;
    CBs::ParamCB       m_param_buffer_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
    ConstantBuffer<CBs::ParamCB> m_param_buffer_data_{};

    StrongShader m_fallback_shader_;

  };
} // namespace Engine::Manager::Graphics
