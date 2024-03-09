#pragma once
#include "pch.h"
#include <egComputeShader.h>

#include "Client.h"

namespace Client::ComputeShaders
{
  class IntersectionCompute final : public Engine::Resources::ComputeShader
  {
  public:

    struct LightTable
    {
      CLIENT_SB_UAV_T(CLIENT_SBUAV_TYPE_INTERSECTION)

      Graphics::OffsetT<int> lightTable[g_max_lights][g_max_lights];
    };

    void OnImGui(const StrongParticleRenderer& pr) override;

  protected:
    void preDispatch() override;
    void postDispatch() override;
    void loadDerived() override;
    void unloadDerived() override;
  };
}

