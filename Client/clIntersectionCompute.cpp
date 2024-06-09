#include "pch.h"
#include "clIntersectionCompute.h"

#include "egRenderPipeline.h"

namespace Client::ComputeShaders
{
  IntersectionCompute::IntersectionCompute()
    : ComputeShader("IntersectionCompute", "cs_intensity_test.hlsl", {32, 32, 1}),
      m_light_table_ptr_(nullptr),
      m_intersection_texture_(nullptr),
      m_position_texture_(nullptr),
      m_target_light_(0) { }

  void IntersectionCompute::OnImGui(const StrongParticleRenderer& pr) {}

  void IntersectionCompute::preDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap)
  {
    if (m_light_table_ptr_ == nullptr)
    {
      return;
    }

    m_light_table_ptr_->CopyUAVHeap(heap);
    m_light_table_ptr_->TransitionToUAV(list);

    // 512 x 512
    SetGroup({256, 1, 1});

    m_intersection_texture_->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 0);
    m_position_texture_->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 1);

    GetRenderPipeline().SetParam<int>(m_target_light_, target_light_slot);
  }

  void IntersectionCompute::postDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap)
  {
    if (m_light_table_ptr_ == nullptr)
    {
      return;
    }

    m_light_table_ptr_->TransitionCommon(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    GetRenderPipeline().SetParam<int>(0, target_light_slot);

    m_intersection_texture_->Unbind(list, BIND_TYPE_SRV);
    m_position_texture_->Unbind(list, BIND_TYPE_SRV);

    m_light_table_ptr_ = nullptr;
    m_target_light_ = 0;
    m_intersection_texture_ = nullptr;
  }

  void IntersectionCompute::loadDerived() {}

  void IntersectionCompute::unloadDerived() {}
}
