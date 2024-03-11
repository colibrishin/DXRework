#include "pch.h"
#include "clIntersectionCompute.h"

#include "egRenderPipeline.h"

namespace Client::ComputeShaders
{
  IntersectionCompute::IntersectionCompute()
    : ComputeShader("IntersectionCompute", "", {32, 32, 1}),
      m_light_table_ptr_(nullptr),
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
    SetGroup({32, 8, 1});

    std::vector<ID3D11ShaderResourceView*> uavs;

    for (const auto& tex : m_intersection_texture_)
    {
      uavs.push_back(tex->GetSRV());
    }

    GetRenderPipeline().BindResources
      (
       BIND_SLOT_UAV_TEXARR,
       SHADER_COMPUTE,
       uavs.data(),
       static_cast<UINT>(uavs.size())
      );

    GetRenderPipeline().SetParam<int>(m_target_light_, target_light_slot);
  }

  void IntersectionCompute::postDispatch()
  {
    if (m_light_table_ptr_ == nullptr)
    {
      return;
    }

    m_light_table_ptr_->UnbindUAV();

    GetRenderPipeline().UnbindResource(BIND_SLOT_UAV_TEXARR, SHADER_COMPUTE);
    GetRenderPipeline().SetParam<int>(0, target_light_slot);

    m_light_table_ptr_ = nullptr;
    m_target_light_ = 0;
    m_intersection_texture_.clear();
  }

  void IntersectionCompute::loadDerived() {}

  void IntersectionCompute::unloadDerived() {}
}
