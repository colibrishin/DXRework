#include "pch.h"
#include "egActor.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::Actor,
                       _ARTAG(m_layer_) _ARTAG(m_local_id_))

namespace Engine::Abstract
{
    eLayerType Actor::GetLayer() const
    {
        return m_layer_;
    }

    WeakScene Actor::GetScene() const
    {
        return m_assigned_scene_;
    }

    ActorID Actor::GetLocalID() const
    {
        return m_local_id_;
    }

    void Actor::OnImGui()
    {
        Renderable::OnImGui();
        ImGui::BulletText("Actor");
        ImGui::Indent(2);
        ImGui::Text("Layer: %d", m_layer_);
        ImGui::Text("Local ID: %lld", m_local_id_);
        ImGui::Unindent(2);
    }

    Actor::Actor()
    : m_assigned_scene_({}),
      m_layer_(LAYER_NONE),
      m_local_id_(g_invalid_id) { }

    void Actor::SetLayer(eLayerType layer)
    {
        m_layer_ = layer;
    }

    void Actor::SetScene(const WeakScene& scene)
    {
        m_assigned_scene_ = scene;
    }

    void Actor::SetLocalID(const ActorID id)
    {
        if (m_assigned_scene_.lock())
        {
            m_local_id_ = id;
        }
    }
} // namespace Engine::Abstract
