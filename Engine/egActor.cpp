#include "pch.h"
#include "egActor.h"

#include "egImGuiHeler.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Abstract::Actor,
 _ARTAG(_BSTSUPER(Renderable))
 _ARTAG(m_layer_)
 _ARTAG(m_local_id_)
)

namespace Engine::Abstract
{
  eLayerType Actor::GetLayer() const { return m_layer_; }

  WeakScene Actor::GetScene() const { return m_assigned_scene_; }

  LocalActorID Actor::GetLocalID() const { return m_local_id_; }

  void Actor::OnImGui()
  {
    Renderable::OnImGui();
    ImGui::BulletText("Actor");
    TextDisabled("Layer", g_layer_type_str[GetLayer()]);
    lldDisabled("Local ID", m_local_id_);
  }

  Actor::Actor()
    : m_assigned_scene_({}),
      m_layer_(LAYER_NONE),
      m_local_id_(g_invalid_id) { }

  void Actor::SetLayer(eLayerType layer) { m_layer_ = layer; }

  void Actor::SetScene(const WeakScene& scene) { m_assigned_scene_ = scene; }

  void Actor::SetLocalID(const LocalActorID id) { if (m_assigned_scene_.lock()) { m_local_id_ = id; } }
} // namespace Engine::Abstract
