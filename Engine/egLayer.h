#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine
{
  class Layer final : public Abstract::Renderable
  {
  public:
    Layer(eLayerType type);

    ~Layer() override = default;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;

    void                 AddGameObject(const StrongObjectBase& obj);
    void                 RemoveGameObject(GlobalEntityID id);
    WeakObjectBase           GetGameObject(GlobalEntityID id) const;
    ConcurrentWeakObjVec GetGameObjects() const;

    auto begin() noexcept { return m_objects_.begin(); }
    auto end() noexcept { return m_objects_.end(); }
    auto begin() const noexcept { return m_objects_.begin(); }
    auto end() const noexcept { return m_objects_.end(); }
    auto cbegin() const noexcept { return m_objects_.cbegin(); }
    auto cend() const noexcept { return m_objects_.cend(); }
    auto size() const noexcept { return m_objects_.size(); }

  private:
    Layer();

    SERIALIZE_DECL

    eLayerType             m_layer_type_;
    std::set<StrongObjectBase> m_objects_;

    // Non-serialized
    ConcurrentWeakObjGlobalMap m_weak_objects_cache_;
  };
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Layer)
