#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
  class Actor : public Renderable
  {
  public:
    ~Actor() override = default;

    eLayerType   GetLayer() const;
    WeakScene    GetScene() const;
    LocalActorID GetLocalID() const;

    void OnImGui() override;

  protected:
    explicit Actor();

  private:
    SERIALIZE_DECL
    friend class Scene;

    void SetLayer(eLayerType layer);
    void SetScene(const WeakScene& scene);
    void SetLocalID(LocalActorID id);

    WeakScene    m_assigned_scene_;
    eLayerType   m_layer_;
    LocalActorID m_local_id_;
  };
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Actor)
BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Actor)
