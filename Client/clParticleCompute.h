#pragma once
#include "egComputeShader.h"

namespace Client::ComputeShaders
{
  class ParticleCompute final : public Engine::Resources::ComputeShader
  {
  public:
    explicit ParticleCompute()
      : ComputeShader("cs_particle", "cs_particle.hlsl", {32, 32, 1}) {}

  protected:
    SERIALIZER_ACCESS

    void preDispatch() override;
    void postDispatch() override;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::ComputeShaders::ParticleCompute)