#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
    class Actor : public Renderable
    {
    public:
        ~Actor() override = default;

        eLayerType GetLayer() const;
        WeakScene  GetScene() const;
        ActorID    GetLocalID() const;

        void OnImGui() override;

    protected:
        explicit Actor();

    private:
        SERIALIZER_ACCESS
        friend class Scene;

        void SetLayer(eLayerType layer);
        void SetScene(const WeakScene& scene);
        void SetLocalID(ActorID id);

        WeakScene  m_assigned_scene_;
        eLayerType m_layer_;
        ActorID    m_local_id_;
    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Actor)
