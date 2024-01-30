#pragma once
#include <algorithm>
#include <array>
#include <random>
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
    void loadDerived() override;
    void unloadDerived() override;

  private:
    std::mt19937_64 getRandomEngine() const;

    constexpr static size_t random_texture_count = 3;
    constexpr static size_t random_value_slot    = 6;
    constexpr static size_t random_texture_size = 500 * 500;

    std::array<std::shared_ptr<Resources::Texture2D>, random_texture_count> m_noises_;

  };
}

BOOST_CLASS_EXPORT_KEY(Client::ComputeShaders::ParticleCompute)