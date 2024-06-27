#pragma once
#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egManager.hpp"

namespace Engine::Manager
{
  class ProjectionFrustum final : public Abstract::Singleton<ProjectionFrustum>
  {
  public:
    explicit ProjectionFrustum(SINGLETON_LOCK_TOKEN)
      : Singleton() {}

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    bool CheckRender(const WeakObjectBase& object) const;

    BoundingFrustum GetFrustum() const;

  private:
    friend struct SingletonDeleter;
    ~ProjectionFrustum() override = default;

    BoundingFrustum m_frustum;
    BoundingSphere  m_sphere;
  };
} // namespace Engine::Manager

REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::ProjectionFrustum)