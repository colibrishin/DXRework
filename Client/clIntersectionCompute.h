#pragma once
#include "pch.h"
#include <egComputeShader.h>

#include "Client.h"
#include "clIntensityTexture.h"
#include "egStructuredBuffer.hpp"

namespace Client::ComputeShaders
{
  class IntersectionCompute final : public Engine::Resources::ComputeShader
  {
  public:
    constexpr static size_t target_light_slot = 1;

    IntersectionCompute();

    struct LightTableSB
    {
      CLIENT_SB_UAV_T(CLIENT_SBUAV_TYPE_INTERSECTION)

      Graphics::OffsetT<int> lightTable[g_max_lights][g_max_lights];
    };

    void OnImGui(const StrongParticleRenderer& pr) override;

    void SetLightTable(Graphics::StructuredBuffer<LightTableSB>* light_table_ptr) { m_light_table_ptr_ = light_table_ptr; }
    void SetIntersectionTexture(const std::vector<Client::Resource::IntensityTexture*>& texs) { m_intersection_texture_ = texs; }
    void SetTargetLight(const UINT target_light) { m_target_light_ = target_light; }


  protected:
    void preDispatch() override;
    void postDispatch() override;
    void loadDerived() override;
    void unloadDerived() override;

  private:
    Graphics::StructuredBuffer<LightTableSB>* m_light_table_ptr_;
    std::vector<Client::Resource::IntensityTexture*> m_intersection_texture_;
    UINT m_target_light_;

  };
}

