#include "pch.h"
#include "clParticleCompute.h"

#include "egApplication.h"
#include "egImGuiHeler.hpp"
#include "egParticleRenderer.h"
#include "egRenderPipeline.h"
#include "egTexture2D.h"

SERIALIZE_IMPL
(
 Client::ComputeShaders::ParticleCompute,
 _ARTAG(_BSTSUPER(ComputeShader))
)

namespace Client::ComputeShaders
{
  void ParticleCompute::preDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap)
  {
    m_noises_[0]->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 0);
    m_noises_[1]->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 1);
    m_noises_[2]->Bind(list, heap, BIND_TYPE_SRV, BIND_SLOT_TEX, 2);

    auto rng = getRandomEngine();
    const auto rng_val = rng() % random_texture_size;

    GetRenderPipeline().SetParam((int)rng_val, random_value_slot);
  }

  void ParticleCompute::postDispatch(ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap)
  {
    m_noises_[0]->Unbind(list, BIND_TYPE_SRV);
    m_noises_[1]->Unbind(list, BIND_TYPE_SRV);
    m_noises_[2]->Unbind(list, BIND_TYPE_SRV);

    auto rng = getRandomEngine();
    std::ranges::shuffle(m_noises_, rng);
  }

  void ParticleCompute::loadDerived()
  {
    static std::vector<std::string> f_name = { "./noise0.dds", "./noise1.dds", "./noise2.dds" };

    auto rng = getRandomEngine();

    std::ranges::shuffle(f_name, rng);

    m_noises_[0] = std::make_shared<Resources::Texture2D>(f_name[0], Resources::Texture::GenericTextureDescription{});
    m_noises_[1] = std::make_shared<Resources::Texture2D>(f_name[1], Resources::Texture::GenericTextureDescription{});
    m_noises_[2] = std::make_shared<Resources::Texture2D>(f_name[2], Resources::Texture::GenericTextureDescription{});

    m_noises_[0]->Load();
    m_noises_[1]->Load();
    m_noises_[2]->Load();
  }

  void ParticleCompute::unloadDerived()
  {
    m_noises_[0].reset();
    m_noises_[1].reset();
    m_noises_[2].reset();
  }

  void ParticleCompute::OnImGui(const StrongParticleRenderer& pr)
  {
    CheckboxAligned("Scaling", getParam(pr).GetParam<bool>(scaling_active_slot));

    if (getParam(pr).GetParam<bool>(scaling_active_slot))
    {
      FloatAligned("Min", getParam(pr).GetParam<float>(scaling_min_slot));
      FloatAligned("Max", getParam(pr).GetParam<float>(scaling_max_slot));
    }

    if (ImGui::Button("Linear Spread"))
    {
      LinearSpread(Vector3(-1.f, 0.f, -1.f), Vector3(1.f, 0.f, 1.f), getInstances(pr), getParam(pr));
    }
  }

  void ParticleCompute::SetScaling(const bool scaling, Graphics::ParamBase& config)
  {
    config.SetParam((int)scaling, scaling_active_slot);
  }

  void ParticleCompute::SetScalingParam(const float min, const float max, Graphics::ParamBase& config)
  {
    config.SetParam(min, scaling_min_slot);
    config.SetParam(max, scaling_max_slot);
  }

  void ParticleCompute::LinearSpread(
    const Vector3& local_min, const Vector3& local_max, InstanceParticles& particles, const Graphics::ParamBase& config
  )
  {
    const auto count = particles.size();
    for (auto i = 0; i < count; ++i)
    {
      auto& instance = particles[i];
      auto  world    = instance.GetWorld().Transpose();

      const auto new_pos = Vector3::Lerp(local_min, local_max, static_cast<float>(i) / static_cast<float>(count));

      world *= Matrix::CreateScale(config.GetParam<float>(Components::ParticleRenderer::size_slot)) * Matrix::CreateTranslation(new_pos - world.Translation());
      instance.SetWorld(world.Transpose());
    }
  }

  std::mt19937_64 ParticleCompute::getRandomEngine() const
  {
    std::random_device dev;
    std::default_random_engine init_seed(dev());

    std::uniform_int_distribution initial_dist
      (
       std::numeric_limits<unsigned long long>().min(),
       std::numeric_limits<unsigned long long>().max()
      );

    std::seed_seq rand_seq
    {
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
      initial_dist(init_seed),
    };

    std::mt19937_64 rng(rand_seq);
    rng.discard(1 << 8);

    return rng;
  }
}
