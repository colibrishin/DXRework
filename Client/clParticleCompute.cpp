#include "pch.h"
#include "clParticleCompute.h"

#include "egApplication.h"
#include "egRenderPipeline.h"
#include "egTexture2D.h"

SERIALIZER_ACCESS_IMPL
(
 Client::ComputeShaders::ParticleCompute,
 _ARTAG(_BSTSUPER(ComputeShader))
)

namespace Client::ComputeShaders
{
  void ParticleCompute::preDispatch()
  {
    m_noises_[0]->BindAs(D3D11_BIND_SHADER_RESOURCE, BIND_SLOT_TEX, 3, SHADER_COMPUTE);
    m_noises_[1]->BindAs(D3D11_BIND_SHADER_RESOURCE, BIND_SLOT_TEX, 4, SHADER_COMPUTE);
    m_noises_[2]->BindAs(D3D11_BIND_SHADER_RESOURCE, BIND_SLOT_TEX, 5, SHADER_COMPUTE);

    m_noises_[0]->PreRender(0.f);
    m_noises_[1]->PreRender(0.f);
    m_noises_[2]->PreRender(0.f);

    m_noises_[0]->Render(0.f);
    m_noises_[1]->Render(0.f);
    m_noises_[2]->Render(0.f);

    auto rng = getRandomEngine();
    const auto rng_val = rng() % random_texture_size;

    GetRenderPipeline().SetParam((int)rng_val, random_value_slot);
  }

  void ParticleCompute::postDispatch()
  {
    m_noises_[0]->PostRender(0.f);
    m_noises_[1]->PostRender(0.f);
    m_noises_[2]->PostRender(0.f);

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
