#pragma once
#include "pch.h"
#include <egComputeShader.h>

#include "Client.h"
#include "clIntensityPositionTexture.h"
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

      Graphics::OffsetT<int> lightTable[g_max_lights];
      Vector4 min[g_max_lights];
      Vector4 max[g_max_lights];

      LightTableSB()
      {
        for (int i = 0; i < g_max_lights; ++i)
        {
          lightTable[i] = 0;
          min[i] = Vector4(FLT_MAX);
          max[i] = Vector4(FLT_MIN);
        }
      }
    };

    void OnImGui(const StrongParticleRenderer& pr) override;

    void SetLightTable(Graphics::StructuredBuffer<LightTableSB>* light_table_ptr) { m_light_table_ptr_ = light_table_ptr; }
    void SetIntersectionTexture(Client::Resource::IntensityTexture& texs) { m_intersection_texture_ = &texs; }
    void SetPositionTexture(Client::Resource::IntensityPositionTexture& texs) { m_position_texture_ = &texs; }
    void SetTargetLight(const UINT target_light) { m_target_light_ = target_light; }


  protected:
    void preDispatch() override;
    void postDispatch() override;
    void loadDerived() override;
    void unloadDerived() override;

  private:
    Graphics::StructuredBuffer<LightTableSB>* m_light_table_ptr_;
    Client::Resource::IntensityTexture* m_intersection_texture_;
    Client::Resource::IntensityPositionTexture* m_position_texture_;
    UINT m_target_light_;

  };
}

