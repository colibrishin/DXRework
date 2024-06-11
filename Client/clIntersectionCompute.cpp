#include "pch.h"
#include "clIntersectionCompute.h"

#include "egRenderPipeline.h"

namespace Client::ComputeShaders
{
  IntersectionCompute::IntersectionCompute()
    : ComputeShader("IntersectionCompute", "cs_intensity_test.hlsl", {32, 32, 1}),
      m_light_table_ptr_(),
      m_intersection_texture_(nullptr),
      m_position_texture_(nullptr),
      m_target_light_(0) { }

  void IntersectionCompute::OnImGui(const StrongParticleRenderer& pr) {}

  void IntersectionCompute::preDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param)
  {
    if (m_light_table_ptr_.expired())
    {
      return;
    }

    const auto& table = m_light_table_ptr_.lock();

    table->CopyUAVHeap(heap);
    table->TransitionToUAV(list);

    // 512 x 512
    SetGroup({256, 1, 1});

    m_intersection_texture_->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 0);
    m_position_texture_->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 1);

    param.SetParam(target_light_slot, static_cast<int>(m_target_light_));
  }

  void IntersectionCompute::postDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param)
  {
    if (m_light_table_ptr_.expired())
    {
      return;
    }

    const auto& table = m_light_table_ptr_.lock();

    table->TransitionCommon(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_intersection_texture_->Unbind(list, BIND_TYPE_SRV);
    m_position_texture_->Unbind(list, BIND_TYPE_SRV);

    m_light_table_ptr_ = {};
    m_target_light_ = 0;
    m_intersection_texture_ = nullptr;
  }

  void IntersectionCompute::loadDerived() {}

  void IntersectionCompute::unloadDerived() {}
}
