#pragma once
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>

#include "egComputeShader.h"

namespace Client::Resources
{
  class ParticleCompute final : public Engine::Resources::ComputeShader
  {
  public:
    explicit ParticleCompute()
      : ComputeShader("cs_particle.hlsl", {32, 32, 1}) {}

  protected:
    SERIALIZER_ACCESS

    void preDispatch() override;
    void postDispatch() override;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Resources::ParticleCompute)