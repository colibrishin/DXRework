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
    SERIALIZE_DECL

    void preDispatch() override;
    void postDispatch() override;
    void loadDerived() override;
    void unloadDerived() override;

    void OnImGui(const StrongParticleRenderer& pr) override;

    void SetScaling(const bool scaling, Graphics::ParamBase& config);
    void SetScalingParam(const float min, const float max, Graphics::ParamBase& config);

    void LinearSpread(const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles, const Graphics::ParamBase& config);

  private:
    // int
    constexpr static size_t scaling_active_slot = 2;
    constexpr static size_t random_value_slot = 3;

    // float
    constexpr static size_t dt_slot             = 3;
    constexpr static size_t scaling_min_slot    = 4;
    constexpr static size_t scaling_max_slot    = 5;

    std::mt19937_64 getRandomEngine() const;

    constexpr static size_t random_texture_count = 3;
    constexpr static size_t random_texture_size = 500 * 500;

    std::array<std::shared_ptr<Resources::Texture2D>, random_texture_count> m_noises_;

  };
}

BOOST_CLASS_EXPORT_KEY(Client::ComputeShaders::ParticleCompute)