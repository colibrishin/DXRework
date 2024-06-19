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
  private:
    struct __declspec(align(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)) ShaderRecord
    {
      byte                        shaderId[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
      //D3D12_GPU_DESCRIPTOR_HANDLE localRootParameters;
    };

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
      ID3D12GraphicsCommandList4 *                              cmd,
      const std::map<WeakModel, std::vector<SBs::InstanceSB>> & instances
    );

    void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);

    void DefaultRootSignature(ID3D12GraphicsCommandList1* cmd) const;
    void DefaultDescriptorHeap(ID3D12GraphicsCommandList1* cmd) const;
    void BindTLAS(ID3D12GraphicsCommandList1* cmd) const;

    ID3D12Device5* GetDevice() const;

  private:
    friend class ToolkitAPI;
    friend class D3Device;
    friend struct SingletonDeleter;
    RaytracingPipeline() = default;
    ~RaytracingPipeline() override;

    void InitializeViewport();
    void InitializeInterface();
    void InitializeSignature();
    void InitializeDescriptorHeaps();

    void InitializeRaytracingPSOTMP();
    void InitializeShaderTable();

    void PrecompileShaders();
    void InitializeOutputBuffer();

    ComPtr<ID3D12Device5> m_device_;
    ComPtr<ID3D12GraphicsCommandList4> m_command_list_;

    ComPtr<ID3D12RootSignature>  m_raytracing_global_signature_;
    ComPtr<ID3D12RootSignature>  m_raytracing_local_signature_;

    ComPtr<ID3D12DescriptorHeap> m_raytracing_buffer_heap_;
    ComPtr<ID3D12DescriptorHeap> m_raytracing_sampler_heap_;

    // Shader parts
    ComPtr<ID3D12StateObject>           m_raytracing_state_object_;
    ComPtr<ID3D12StateObjectProperties> m_raytracing_state_object_properties_;
    ComPtr<ID3D12Resource>              m_raygen_shader_table_;
    ComPtr<ID3D12Resource>              m_closest_hit_shader_table_;
    ComPtr<ID3D12Resource>              m_miss_shader_table_;

    ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_;

    ComPtr<ID3D12Resource> m_output_buffer_;
    ComPtr<ID3D12DescriptorHeap> m_output_uav_heap_;

    UINT m_buffer_descriptor_size_ = 0;
    UINT m_sampler_descriptor_size_ = 0;

    AccelStructBuffer m_tlas_;

    D3D12_VIEWPORT m_viewport_{};
    D3D12_RECT    m_scissor_rect_{};

    StrongShader m_fallback_shader_;

  };
} // namespace Engine::Manager::Graphics
