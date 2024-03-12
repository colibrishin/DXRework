#include "pch.h"
#include "clIntersectionCompute.h"

#include "egRenderPipeline.h"

namespace Client::ComputeShaders
{
  IntersectionCompute::IntersectionCompute()
    : ComputeShader("IntersectionCompute", "cs_intensity_test.hlsl", {32, 32, 1}),
      m_light_table_ptr_(nullptr),
      m_intersection_texture_(nullptr),
      m_target_light_(0) { }

  void IntersectionCompute::OnImGui(const StrongParticleRenderer& pr) {}

  void IntersectionCompute::preDispatch()
  {
    if (m_light_table_ptr_ == nullptr)
    {
      return;
    }

    m_light_table_ptr_->BindUAV();

    // 512 x 512
    SetGroup({256, 1, 1});

    ComPtr<ID3D11ShaderResourceView> srv = m_intersection_texture_->GetSRV();

    GetRenderPipeline().BindResource(BIND_SLOT_TEX, SHADER_COMPUTE, srv.GetAddressOf());

    GetRenderPipeline().SetParam<int>(m_target_light_, target_light_slot);
  }

  void IntersectionCompute::postDispatch()
  {
    if (m_light_table_ptr_ == nullptr)
    {
      return;
    }

    m_light_table_ptr_->UnbindUAV();

    GetRenderPipeline().UnbindResource(BIND_SLOT_TEX, SHADER_COMPUTE);
    GetRenderPipeline().SetParam<int>(0, target_light_slot);

    m_light_table_ptr_ = nullptr;
    m_target_light_ = 0;
    m_intersection_texture_ = nullptr;
  }

  void IntersectionCompute::loadDerived() {}

  void IntersectionCompute::unloadDerived() {}
}
