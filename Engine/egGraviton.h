#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Physics
{
  class Graviton : public Abstract::Singleton<Graviton>
  {
  public:
    Graviton(SINGLETON_LOCK_TOKEN) {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void Initialize() override;

  private:
    friend struct SingletonDeleter;
    ~Graviton() override = default;

  };
}
